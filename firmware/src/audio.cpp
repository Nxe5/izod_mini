#include "audio.h"
#include "driver/i2s.h"

#ifndef I2S_BCLK
#define I2S_BCLK 39
#endif
#ifndef I2S_WS
#define I2S_WS 38
#endif
#ifndef I2S_DATA
#define I2S_DATA 11
#endif

static const int kAudioSampleRateHz = 44100;
static const int AUDIO_FREQ_HZ = 1000;  // 1 kHz tone
static volatile bool s_audioPlaying = false;
static volatile int s_audioVolume = 50;   // 0-100 (default 50%)
static TaskHandle_t s_audioTaskHandle = NULL;

static int16_t sineTable[256];

static void buildSineTable() {
    for (int i = 0; i < 256; i++) {
        float phase = (2.0f * PI * i) / 256.0f;
        sineTable[i] = (int16_t)(sin(phase) * 32767);
    }
}

static void fillSineStereo(int16_t* buf, int frames, int freq_hz, int volume_percent) {
    static uint32_t phase = 0;
    const uint32_t phase_inc = (uint32_t)((uint64_t)freq_hz * 256ull / kAudioSampleRateHz * 65536ull);
    const int32_t amp = (int32_t)(volume_percent * 32767 / 100);
    for (int i = 0; i < frames; i++) {
        phase += phase_inc;
        uint8_t idx = (phase >> 16) & 0xFF;
        int16_t s = (int16_t)((sineTable[idx] * amp) / 32767);
        buf[i * 2 + 0] = s; // Left
        buf[i * 2 + 1] = s; // Right
    }
}

static void audioTask(void* pv) {
    const int frames = 256;
    int16_t buffer[frames * 2];
    buildSineTable();
    for (;;) {
        if (s_audioPlaying) {
            fillSineStereo(buffer, frames, AUDIO_FREQ_HZ, s_audioVolume);
            size_t bytes_written = 0;
            i2s_write(I2S_NUM_0, buffer, frames * 2 * sizeof(int16_t), &bytes_written, portMAX_DELAY);
        } else {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
}

static bool initI2SOutput() {
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = kAudioSampleRateHz,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = 0,
        .dma_buf_count = 8,
        .dma_buf_len = 256,
        .use_apll = true,
        .tx_desc_auto_clear = true,
        .fixed_mclk = 0
    };
    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_BCLK,
        .ws_io_num = I2S_WS,
        .data_out_num = I2S_DATA,
        .data_in_num = I2S_PIN_NO_CHANGE
    };
    if (i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL) != ESP_OK) return false;
    if (i2s_set_pin(I2S_NUM_0, &pin_config) != ESP_OK) return false;
    if (i2s_set_clk(I2S_NUM_0, kAudioSampleRateHz, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_STEREO) != ESP_OK) return false;
    return true;
}

bool audioInit() {
    if (!initI2SOutput()) return false;
    xTaskCreatePinnedToCore(audioTask, "AudioTask", 4096, NULL, 2, &s_audioTaskHandle, 0);
    return true;
}

void audioSetPlaying(bool play) { s_audioPlaying = play; }
bool audioIsPlaying() { return s_audioPlaying; }
void audioSetVolume(int percent) { s_audioVolume = constrain(percent, 0, 100); }
int audioGetVolume() { return s_audioVolume; }
