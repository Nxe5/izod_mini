#include "audio_mp3.h"
#include "audio.h"
#include <SD.h>
#include <AudioFileSourceSD.h>
#include <AudioGeneratorMP3.h>
#include <AudioOutputI2S.h>

#ifndef I2S_BCLK
#define I2S_BCLK 39
#endif
#ifndef I2S_WS
#define I2S_WS 38
#endif
#ifndef I2S_DATA
#define I2S_DATA 11
#endif

static AudioFileSourceSD *mp3file = nullptr;
static AudioGeneratorMP3 *mp3 = nullptr;
static AudioOutputI2S *out = nullptr;
static volatile bool s_stopReq = false;
static int s_lastVolumePercent = -1;

static void stopAll() {
  if (mp3) { mp3->stop(); delete mp3; mp3 = nullptr; }
  if (mp3file) { delete mp3file; mp3file = nullptr; }
  if (out) { delete out; out = nullptr; }
}

bool mp3StartFile(const char* path) {
  stopAll();
  s_stopReq = false;
  Serial.printf("MP3: opening %s\n", path);
  mp3file = new AudioFileSourceSD(path);
  if (!mp3file || !mp3file->isOpen()) { Serial.println("MP3: file open failed"); stopAll(); return false; }
  out = new AudioOutputI2S();
  out->SetPinout(I2S_BCLK, I2S_WS, I2S_DATA);
  out->SetChannels(2);
  {
    int vol = audioGetVolume();
    float gain = constrain(vol, 0, 100) / 100.0f;
    out->SetGain(gain);
    s_lastVolumePercent = vol;
  }
  Serial.printf("MP3: I2S pins BCK=%d LRCK=%d DATA=%d\n", I2S_BCLK, I2S_WS, I2S_DATA);
  mp3 = new AudioGeneratorMP3();
  if (!mp3->begin(mp3file, out)) { Serial.println("MP3: decoder begin failed"); stopAll(); return false; }
  Serial.println("MP3: decoder running");
  return true;
}

bool mp3StartFirstUnderMusic() {
  File root = SD.open("/Music"); if (!root || !root.isDirectory()) { if (root) root.close(); return false; }
  File f; bool ok=false;
  while ((f = root.openNextFile())) {
    if (!f.isDirectory()) {
      String name = f.name(); name.toLowerCase();
      if (name.endsWith(".mp3")) { String p = String("/Music/") + f.name(); f.close(); ok = mp3StartFile(p.c_str()); break; }
      else { Serial.printf("MP3: skipping %s\n", f.name()); }
    }
    f.close();
  }
  root.close();
  if (!ok) Serial.println("MP3: no MP3 found under /Music");
  return ok;
}

bool mp3IsPlaying() { return mp3 && mp3->isRunning(); }
void mp3Stop() { Serial.println("MP3: stop requested"); s_stopReq = true; }

// Polling function to be called from a task or main loop
static void mp3PollTask(void* pv) {
  for(;;) {
    if (s_stopReq) { Serial.println("MP3: stop handling"); stopAll(); s_stopReq = false; }
    if (mp3 && mp3->isRunning()) {
      // Apply volume changes to I2S output gain
      if (out) {
        int vol = audioGetVolume();
        if (vol != s_lastVolumePercent) {
          float gain = constrain(vol, 0, 100) / 100.0f;
          out->SetGain(gain);
          s_lastVolumePercent = vol;
        }
      }
      if (!mp3->loop()) { Serial.println("MP3: finished or error in loop"); stopAll(); }
      // Yield cooperatively to avoid starving other tasks
      vTaskDelay(pdMS_TO_TICKS(1));
    } else {
      vTaskDelay(pdMS_TO_TICKS(20));
    }
  }
}

static TaskHandle_t s_mp3Task = nullptr;

void mp3EnsureTask() {
  if (!s_mp3Task) xTaskCreatePinnedToCore(mp3PollTask, "MP3Task", 6144, NULL, 1, &s_mp3Task, 0);
}
