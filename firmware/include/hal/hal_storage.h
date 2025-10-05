/*
 * Hardware Abstraction Layer - Storage Interface
 * Abstracts SD card operations for both ESP32 hardware and host emulation
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Storage configuration constants
#define HAL_STORAGE_MAX_PATH_LENGTH     256
#define HAL_STORAGE_MAX_FILENAME_LENGTH 64
#define HAL_STORAGE_MAX_OPEN_FILES      8

// File access modes
typedef enum {
    HAL_STORAGE_MODE_READ       = 0x01,
    HAL_STORAGE_MODE_WRITE      = 0x02,
    HAL_STORAGE_MODE_APPEND     = 0x04,
    HAL_STORAGE_MODE_CREATE     = 0x08,
    HAL_STORAGE_MODE_TRUNCATE   = 0x10,
    HAL_STORAGE_MODE_BINARY     = 0x20
} hal_storage_mode_t;

// File seek origins
typedef enum {
    HAL_STORAGE_SEEK_SET = 0,   // From beginning of file
    HAL_STORAGE_SEEK_CUR = 1,   // From current position
    HAL_STORAGE_SEEK_END = 2    // From end of file
} hal_storage_seek_t;

// File types
typedef enum {
    HAL_STORAGE_TYPE_UNKNOWN    = 0,
    HAL_STORAGE_TYPE_FILE       = 1,
    HAL_STORAGE_TYPE_DIRECTORY  = 2,
    HAL_STORAGE_TYPE_SYMLINK    = 3
} hal_storage_file_type_t;

// Storage information
typedef struct {
    uint64_t total_bytes;       // Total storage capacity
    uint64_t used_bytes;        // Used storage space
    uint64_t free_bytes;        // Free storage space
    uint32_t total_files;       // Total number of files
    uint32_t total_dirs;        // Total number of directories
    bool read_only;             // Storage is read-only
    bool mounted;               // Storage is mounted
    char label[32];             // Volume label
    char filesystem[16];        // Filesystem type (FAT32, etc.)
} hal_storage_info_t;

// File information
typedef struct {
    char name[HAL_STORAGE_MAX_FILENAME_LENGTH];
    char path[HAL_STORAGE_MAX_PATH_LENGTH];
    hal_storage_file_type_t type;
    uint64_t size;              // File size in bytes
    uint32_t created_time;      // Creation timestamp
    uint32_t modified_time;     // Last modification timestamp
    uint32_t accessed_time;     // Last access timestamp
    bool read_only;             // File is read-only
    bool hidden;                // File is hidden
    bool system;                // System file
} hal_storage_file_info_t;

// Directory iterator handle
typedef void* hal_storage_dir_t;

// File handle
typedef void* hal_storage_file_t;

// Storage initialization and control
bool hal_storage_init(void);
void hal_storage_deinit(void);
bool hal_storage_is_initialized(void);

// Mount/unmount operations
bool hal_storage_mount(void);
bool hal_storage_unmount(void);
bool hal_storage_is_mounted(void);

// Storage information
bool hal_storage_get_info(hal_storage_info_t* info);
uint64_t hal_storage_get_total_space(void);
uint64_t hal_storage_get_free_space(void);
uint64_t hal_storage_get_used_space(void);

// File operations
hal_storage_file_t hal_storage_open(const char* path, hal_storage_mode_t mode);
void hal_storage_close(hal_storage_file_t file);
bool hal_storage_is_open(hal_storage_file_t file);

// File I/O operations
size_t hal_storage_read(hal_storage_file_t file, void* buffer, size_t size);
size_t hal_storage_write(hal_storage_file_t file, const void* buffer, size_t size);
bool hal_storage_flush(hal_storage_file_t file);
bool hal_storage_sync(hal_storage_file_t file);

// File positioning
bool hal_storage_seek(hal_storage_file_t file, int64_t offset, hal_storage_seek_t origin);
int64_t hal_storage_tell(hal_storage_file_t file);
bool hal_storage_rewind(hal_storage_file_t file);
bool hal_storage_eof(hal_storage_file_t file);

// File information and properties
bool hal_storage_get_file_info(const char* path, hal_storage_file_info_t* info);
uint64_t hal_storage_get_file_size(const char* path);
uint64_t hal_storage_get_file_size_handle(hal_storage_file_t file);
bool hal_storage_file_exists(const char* path);

// File management
bool hal_storage_delete_file(const char* path);
bool hal_storage_rename_file(const char* old_path, const char* new_path);
bool hal_storage_copy_file(const char* src_path, const char* dst_path);
bool hal_storage_move_file(const char* src_path, const char* dst_path);

// Directory operations
bool hal_storage_create_dir(const char* path);
bool hal_storage_delete_dir(const char* path);
bool hal_storage_dir_exists(const char* path);

// Directory listing
hal_storage_dir_t hal_storage_open_dir(const char* path);
void hal_storage_close_dir(hal_storage_dir_t dir);
bool hal_storage_read_dir(hal_storage_dir_t dir, hal_storage_file_info_t* info);
void hal_storage_rewind_dir(hal_storage_dir_t dir);

// Path utilities
bool hal_storage_is_absolute_path(const char* path);
bool hal_storage_join_path(char* result, size_t result_size, const char* base, const char* relative);
bool hal_storage_get_parent_dir(const char* path, char* parent, size_t parent_size);
bool hal_storage_get_filename(const char* path, char* filename, size_t filename_size);
bool hal_storage_get_extension(const char* path, char* extension, size_t extension_size);

// Convenience functions for common file operations
bool hal_storage_read_file_to_buffer(const char* path, void** buffer, size_t* size);
bool hal_storage_write_buffer_to_file(const char* path, const void* buffer, size_t size);
bool hal_storage_append_to_file(const char* path, const void* buffer, size_t size);

// Text file operations
char* hal_storage_read_line(hal_storage_file_t file, char* buffer, size_t buffer_size);
bool hal_storage_write_line(hal_storage_file_t file, const char* line);
bool hal_storage_printf(hal_storage_file_t file, const char* format, ...);

// Binary file operations
bool hal_storage_read_uint8(hal_storage_file_t file, uint8_t* value);
bool hal_storage_read_uint16(hal_storage_file_t file, uint16_t* value);
bool hal_storage_read_uint32(hal_storage_file_t file, uint32_t* value);
bool hal_storage_write_uint8(hal_storage_file_t file, uint8_t value);
bool hal_storage_write_uint16(hal_storage_file_t file, uint16_t value);
bool hal_storage_write_uint32(hal_storage_file_t file, uint32_t value);

// File searching and filtering
typedef bool (*hal_storage_filter_t)(const hal_storage_file_info_t* info, void* user_data);
bool hal_storage_find_files(const char* path, const char* pattern, 
                           hal_storage_file_info_t* results, size_t max_results, size_t* found_count);
bool hal_storage_find_files_filtered(const char* path, hal_storage_filter_t filter, void* user_data,
                                     hal_storage_file_info_t* results, size_t max_results, size_t* found_count);

// Batch operations
bool hal_storage_delete_files_pattern(const char* path, const char* pattern);
bool hal_storage_copy_directory(const char* src_path, const char* dst_path);

// File watching/monitoring (if supported)
typedef void (*hal_storage_watch_callback_t)(const char* path, uint32_t events, void* user_data);
bool hal_storage_watch_file(const char* path, hal_storage_watch_callback_t callback, void* user_data);
bool hal_storage_unwatch_file(const char* path);

// Performance and caching
void hal_storage_set_cache_size(size_t size);
size_t hal_storage_get_cache_size(void);
void hal_storage_flush_cache(void);
void hal_storage_enable_write_cache(bool enabled);

// Performance monitoring
void hal_storage_get_stats(uint32_t* reads, uint32_t* writes, uint32_t* cache_hits, uint32_t* cache_misses);
void hal_storage_reset_stats(void);

// Hardware-specific operations (SD card)
bool hal_storage_sd_detect(void);              // Check if SD card is inserted
bool hal_storage_sd_get_info(uint32_t* capacity_mb, uint32_t* speed_class);
bool hal_storage_sd_format(const char* filesystem); // Format SD card
bool hal_storage_sd_benchmark(uint32_t* read_speed_kbps, uint32_t* write_speed_kbps);

// Error handling
typedef enum {
    HAL_STORAGE_ERROR_NONE = 0,
    HAL_STORAGE_ERROR_INIT_FAILED = 1,
    HAL_STORAGE_ERROR_NOT_MOUNTED = 2,
    HAL_STORAGE_ERROR_FILE_NOT_FOUND = 3,
    HAL_STORAGE_ERROR_ACCESS_DENIED = 4,
    HAL_STORAGE_ERROR_DISK_FULL = 5,
    HAL_STORAGE_ERROR_READ_ONLY = 6,
    HAL_STORAGE_ERROR_INVALID_PATH = 7,
    HAL_STORAGE_ERROR_IO_ERROR = 8,
    HAL_STORAGE_ERROR_CORRUPTED = 9,
    HAL_STORAGE_ERROR_NO_CARD = 10
} hal_storage_error_t;

hal_storage_error_t hal_storage_get_last_error(void);
const char* hal_storage_get_error_string(hal_storage_error_t error);

// Self-test and diagnostics
bool hal_storage_self_test(void);
bool hal_storage_check_filesystem(bool repair);

#ifdef __cplusplus
}
#endif

