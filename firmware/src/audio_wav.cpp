#include "audio_wav.h"
#include "audio.h"
#include <SD.h>
#include "driver/i2s.h"

static File s_wav;
static TaskHandle_t s_wavTask = NULL;
static volatile bool s_wavPlaying = false;
static volatile uint16_t s_bits = 16;
static volatile uint16_t s_channels = 2;
static volatile uint32_t s_rate = 44100;

struct WavHeader {
  char riff[4];
  uint32_t size;
  char wave[4];
};

static bool readHeaderAndSeekData(File &f, uint16_t &bits, uint16_t &channels, uint32_t &rate) {
  WavHeader h;
  if (f.read((uint8_t*)&h, sizeof(h)) != sizeof(h)) return false;
  if (strncmp(h.riff, "RIFF", 4) != 0 || strncmp(h.wave, "WAVE", 4) != 0) return false;
  bool haveFmt = false;
  while (f.available()) {
    char id[4]; uint32_t len;
    if (f.read((uint8_t*)id, 4) != 4) return false;
    if (f.read((uint8_t*)&len, 4) != 4) return false;
    if (strncmp(id, "fmt ", 4) == 0) {
      uint16_t audioFmt, ch; uint32_t sr, br; uint16_t ba, bps;
      if (len < 16) return false;
      f.read((uint8_t*)&audioFmt, 2);
      f.read((uint8_t*)&ch, 2);
      f.read((uint8_t*)&sr, 4);
      f.read((uint8_t*)&br, 4);
      f.read((uint8_t*)&ba, 2);
      f.read((uint8_t*)&bps, 2);
      if (len > 16) f.seek(f.position() + (len - 16));
      if (audioFmt != 1) return false; // PCM only
      bits = bps; channels = ch; rate = sr; haveFmt = true;
    } else if (strncmp(id, "data", 4) == 0) {
      // Require fmt first
      if (!haveFmt) return false;
      return true;
    } else {
      f.seek(f.position() + len);
    }
  }
  return false;
}

static void wavTask(void* pv) {
  // Mono expand requires bigger out buffer
  static uint8_t inBuf[1024];
  static uint8_t outBuf[2048];
  while (1) {
    if (s_wavPlaying && s_wav) {
      int n = s_wav.read(inBuf, sizeof(inBuf));
      if (n <= 0) { s_wavPlaying = false; s_wav.close(); continue; }
      size_t wrote = 0;
      if (s_bits == 16 && s_channels == 2) {
        // Apply software volume scaling
        int volumePercent = audioGetVolume();
        if (volumePercent < 100) {
          int samples = n / 2; // int16_t samples
          int16_t* p = (int16_t*)inBuf;
          for (int i = 0; i < samples; i++) {
            int32_t s = p[i];
            s = (s * volumePercent) / 100;
            p[i] = (int16_t)s;
          }
        }
        i2s_write(I2S_NUM_0, inBuf, n, &wrote, portMAX_DELAY);
      } else if (s_bits == 16 && s_channels == 1) {
        // Duplicate mono to stereo
        int samples = n / 2; // int16_t samples
        int16_t* in16 = (int16_t*)inBuf;
        int16_t* out16 = (int16_t*)outBuf;
        int volumePercent = audioGetVolume();
        for (int i = 0; i < samples; i++) {
          int32_t s = in16[i];
          s = (s * volumePercent) / 100;
          out16[i*2 + 0] = s;
          out16[i*2 + 1] = s;
        }
        i2s_write(I2S_NUM_0, outBuf, samples * 2 /*stereo*/ * 2 /*bytes*/ , &wrote, portMAX_DELAY);
      } else {
        // Unsupported format in stream; stop
        s_wavPlaying = false; s_wav.close();
      }
    } else {
      vTaskDelay(pdMS_TO_TICKS(10));
    }
  }
}

bool wavStartFile(const char* path) {
  if (s_wav) { s_wav.close(); }
  File f = SD.open(path, FILE_READ);
  if (!f) return false;
  uint16_t bits=0, ch=0; uint32_t rate=0;
  if (!readHeaderAndSeekData(f, bits, ch, rate)) { f.close(); return false; }
  // Accept 16-bit, mono or stereo, 44100 or 48000 Hz
  if (bits != 16) { f.close(); return false; }
  if (!(ch == 1 || ch == 2)) { f.close(); return false; }
  if (!(rate == 44100 || rate == 48000)) { f.close(); return false; }
  s_bits = bits; s_channels = ch; s_rate = rate;
  // Reconfigure I2S clock to match file
  i2s_set_clk(I2S_NUM_0, s_rate, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_STEREO);
  s_wav = f;
  audioSetPlaying(false); // stop tone
  if (!s_wavTask) xTaskCreatePinnedToCore(wavTask, "WavTask", 4096, NULL, 2, &s_wavTask, 0);
  s_wavPlaying = true;
  return true;
}

bool wavStartFirstUnderMusic() {
  File root = SD.open("/Music"); if (!root || !root.isDirectory()) { if (root) root.close(); return false; }
  File f; bool ok=false;
  while ((f = root.openNextFile())) {
    if (!f.isDirectory()) {
      String name = f.name(); name.toLowerCase();
      if (name.endsWith(".wav")) { String p = String("/Music/") + f.name(); f.close(); ok = wavStartFile(p.c_str()); break; }
    }
    f.close();
  }
  root.close();
  return ok;
}

void wavStop() { s_wavPlaying = false; if (s_wav) s_wav.close(); }
bool wavIsPlaying() { return s_wavPlaying; }
