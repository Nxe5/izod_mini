/*
 * Hardware Abstraction Layer - Audio Interface
 * Abstracts PCM5102A audio operations for both ESP32 hardware and host emulation
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Audio configuration constants
#define HAL_AUDIO_SAMPLE_RATE_8KHZ   8000
#define HAL_AUDIO_SAMPLE_RATE_16KHZ  16000
#define HAL_AUDIO_SAMPLE_RATE_22KHZ  22050
#define HAL_AUDIO_SAMPLE_RATE_44KHZ  44100
#define HAL_AUDIO_SAMPLE_RATE_48KHZ  48000

#define HAL_AUDIO_DEFAULT_SAMPLE_RATE HAL_AUDIO_SAMPLE_RATE_44KHZ
#define HAL_AUDIO_MAX_VOLUME         100
#define HAL_AUDIO_DEFAULT_VOLUME     50

// Audio formats
typedef enum {
    HAL_AUDIO_FORMAT_PCM_8BIT_MONO   = 0,
    HAL_AUDIO_FORMAT_PCM_8BIT_STEREO = 1,
    HAL_AUDIO_FORMAT_PCM_16BIT_MONO  = 2,
    HAL_AUDIO_FORMAT_PCM_16BIT_STEREO = 3,
    HAL_AUDIO_FORMAT_MP3             = 4,
    HAL_AUDIO_FORMAT_WAV             = 5
} hal_audio_format_t;

// Audio playback states
typedef enum {
    HAL_AUDIO_STATE_STOPPED = 0,
    HAL_AUDIO_STATE_PLAYING = 1,
    HAL_AUDIO_STATE_PAUSED  = 2,
    HAL_AUDIO_STATE_ERROR   = 3
} hal_audio_state_t;

// Audio source types
typedef enum {
    HAL_AUDIO_SOURCE_BUFFER = 0,    // Play from memory buffer
    HAL_AUDIO_SOURCE_FILE   = 1,    // Play from file
    HAL_AUDIO_SOURCE_STREAM = 2     // Play from stream/URL
} hal_audio_source_t;

// Audio configuration structure
typedef struct {
    uint32_t sample_rate;           // Sample rate in Hz
    hal_audio_format_t format;      // Audio format
    uint8_t volume;                 // Volume (0-100)
    bool loop;                      // Loop playback
    uint16_t buffer_size;           // Buffer size in samples
} hal_audio_config_t;

// Audio callback function types
typedef void (*hal_audio_callback_t)(void* user_data);
typedef void (*hal_audio_data_callback_t)(int16_t* buffer, size_t samples, void* user_data);

// Audio initialization and control
bool hal_audio_init(const hal_audio_config_t* config);
void hal_audio_deinit(void);
bool hal_audio_is_initialized(void);

// Configuration management
bool hal_audio_set_config(const hal_audio_config_t* config);
void hal_audio_get_config(hal_audio_config_t* config);
bool hal_audio_set_sample_rate(uint32_t sample_rate);
uint32_t hal_audio_get_sample_rate(void);

// Volume control
void hal_audio_set_volume(uint8_t volume);      // 0-100
uint8_t hal_audio_get_volume(void);
void hal_audio_set_mute(bool muted);
bool hal_audio_is_muted(void);

// Playback control
bool hal_audio_play_buffer(const void* buffer, size_t size, hal_audio_format_t format);
bool hal_audio_play_file(const char* filename);
bool hal_audio_play_stream(const char* url);
void hal_audio_stop(void);
void hal_audio_pause(void);
void hal_audio_resume(void);

// Playback state
hal_audio_state_t hal_audio_get_state(void);
bool hal_audio_is_playing(void);
bool hal_audio_is_paused(void);

// Position and timing
uint32_t hal_audio_get_position_ms(void);       // Current position in milliseconds
uint32_t hal_audio_get_duration_ms(void);       // Total duration in milliseconds
bool hal_audio_seek_to_ms(uint32_t position_ms);
float hal_audio_get_progress(void);             // Progress as 0.0-1.0

// Loop control
void hal_audio_set_loop(bool loop);
bool hal_audio_get_loop(void);

// Callback management
void hal_audio_set_end_callback(hal_audio_callback_t callback, void* user_data);
void hal_audio_set_data_callback(hal_audio_data_callback_t callback, void* user_data);
void hal_audio_clear_callbacks(void);

// Buffer management for streaming
bool hal_audio_write_samples(const int16_t* samples, size_t count);
size_t hal_audio_get_buffer_free_space(void);
size_t hal_audio_get_buffer_used_space(void);
void hal_audio_flush_buffer(void);

// Audio effects and processing
void hal_audio_set_equalizer(const float* bands, size_t band_count);  // 10-band EQ
void hal_audio_set_bass_boost(float boost);    // Bass boost in dB
void hal_audio_set_treble_boost(float boost);  // Treble boost in dB
void hal_audio_reset_effects(void);

// Format detection and conversion
hal_audio_format_t hal_audio_detect_format(const void* data, size_t size);
bool hal_audio_convert_format(const void* input, size_t input_size, 
                              hal_audio_format_t input_format,
                              void* output, size_t* output_size,
                              hal_audio_format_t output_format);

// Audio analysis
float hal_audio_get_peak_level(void);          // Peak level 0.0-1.0
float hal_audio_get_rms_level(void);           // RMS level 0.0-1.0
void hal_audio_get_spectrum(float* spectrum, size_t bins);  // FFT spectrum

// Hardware-specific controls (PCM5102A)
void hal_audio_set_soft_mute(bool muted);      // Soft mute
void hal_audio_set_deemphasis(bool enabled);   // De-emphasis filter
void hal_audio_set_filter_mode(uint8_t mode);  // Digital filter mode

// Performance and debugging
void hal_audio_get_stats(uint32_t* samples_played, uint32_t* underruns, uint32_t* overruns);
void hal_audio_reset_stats(void);
bool hal_audio_self_test(void);                // Hardware self-test

// Error handling
typedef enum {
    HAL_AUDIO_ERROR_NONE = 0,
    HAL_AUDIO_ERROR_INIT_FAILED = 1,
    HAL_AUDIO_ERROR_INVALID_FORMAT = 2,
    HAL_AUDIO_ERROR_BUFFER_FULL = 3,
    HAL_AUDIO_ERROR_BUFFER_EMPTY = 4,
    HAL_AUDIO_ERROR_FILE_NOT_FOUND = 5,
    HAL_AUDIO_ERROR_DECODE_FAILED = 6,
    HAL_AUDIO_ERROR_HARDWARE_FAULT = 7
} hal_audio_error_t;

hal_audio_error_t hal_audio_get_last_error(void);
const char* hal_audio_get_error_string(hal_audio_error_t error);

#ifdef __cplusplus
}
#endif

