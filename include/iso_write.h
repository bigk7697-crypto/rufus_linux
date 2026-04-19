/*
 * Rufus Linux - ISO Writing Module
 * Copyright © 2026 Port Project
 *
 * ISO image writing to USB devices
 */

#ifndef ISO_WRITE_H
#define ISO_WRITE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define MAX_ISO_PATH 512
#define WRITE_BUFFER_SIZE (1024 * 1024)  // 1MB buffer
#define MAX_PROGRESS_MARKERS 80

typedef struct {
    char filepath[MAX_ISO_PATH];
    uint64_t size_bytes;
    uint64_t size_sectors;
    bool is_valid;
    bool is_bootable;
    char volume_id[33];
    char publisher[129];
    char application[129];
} iso_info_t;

typedef struct {
    uint64_t bytes_written;
    uint64_t total_bytes;
    float percent_complete;
    int current_marker;
    char progress_bar[MAX_PROGRESS_MARKERS + 1];
} write_progress_t;

typedef enum {
    WRITE_SUCCESS = 0,
    WRITE_ERROR_INVALID_ISO = -1,
    WRITE_ERROR_INVALID_DEVICE = -2,
    WRITE_ERROR_OPEN_ISO = -3,
    WRITE_ERROR_OPEN_DEVICE = -4,
    WRITE_ERROR_READ_ISO = -5,
    WRITE_ERROR_WRITE_DEVICE = -6,
    WRITE_ERROR_SYNC = -7,
    WRITE_ERROR_CANCELLED = -8,
    WRITE_ERROR_SPACE = -9
} write_result_t;

// Initialize ISO writing system
bool iso_write_init(void);

// Cleanup ISO writing system
void iso_write_cleanup(void);

// Get ISO file information
bool get_iso_info(const char* iso_path, iso_info_t* iso_info);

// Validate ISO file
bool validate_iso_file(const char* iso_path);

// Write ISO to device
write_result_t write_iso_to_device(const char* iso_path, const char* device_path, 
                                  write_progress_t* progress);

// Update progress display
void update_progress(write_progress_t* progress, uint64_t bytes_written, uint64_t total_bytes);

// Print ISO information
void print_iso_info(const iso_info_t* iso_info);

// Check if device has enough space for ISO
bool check_device_space(const char* device_path, const iso_info_t* iso_info);

// Create progress bar string
void create_progress_bar(float percent, char* buffer, size_t buffer_size);

// Cancel ongoing write operation (thread-safe)
void cancel_write_operation(void);

// Check if write operation was cancelled
bool is_write_cancelled(void);

#endif // ISO_WRITE_H
