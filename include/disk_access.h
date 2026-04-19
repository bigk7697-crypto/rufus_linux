/*
 * Rufus Linux - Disk Access Module
 * Copyright © 2026 Port Project
 *
 * Safe disk selection and access for Linux
 */

#ifndef DISK_ACCESS_H
#define DISK_ACCESS_H

#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>

#define MAX_DISK_PATH 256
#define MAX_DISK_SIZE 128ULL * 1024 * 1024 * 1024 * 1024  // 128 TB max
#define MIN_DISK_SIZE 8ULL * 1024 * 1024  // 8 MB min
#define SECTOR_SIZE 512
#define MBR_SIZE 512
#define MBR_BACKUP_SIZE 1024  // Extra space for safety

typedef struct {
    char device_path[MAX_DISK_PATH];
    char model[256];
    char vendor[256];
    char serial[256];
    uint64_t size_bytes;
    uint64_t size_sectors;
    bool is_removable;
    bool is_usb;
    bool is_system_disk;
    bool has_partitions;
    int partition_count;
    char mount_points[16][256];  // Up to 16 mount points
    int mount_point_count;
} disk_info_t;

typedef struct {
    uint8_t data[MBR_BACKUP_SIZE];
    size_t size;
    bool valid;
} mbr_backup_t;

// Initialize disk access system
bool disk_access_init(void);

// Cleanup disk access system
void disk_access_cleanup(void);

// Get disk information from device path
bool get_disk_info(const char* device_path, disk_info_t* disk_info);

// Validate device path (exists, accessible, block device)
bool validate_device_path(const char* device_path);

// Check if device is safe to use (not system disk)
bool is_safe_device(const disk_info_t* disk_info);

// Check if device is mounted
bool is_device_mounted(const char* device_path);

// Unmount device (all partitions)
bool unmount_device(const char* device_path);

// Create MBR backup before writing
bool create_mbr_backup(const char* device_path, mbr_backup_t* backup);

// Restore MBR from backup
bool restore_mbr_backup(const char* device_path, const mbr_backup_t* backup);

// Save MBR backup to file
bool save_mbr_backup_to_file(const mbr_backup_t* backup, const char* filename);

// Load MBR backup from file
bool load_mbr_backup_from_file(const char* filename, mbr_backup_t* backup);

// Print disk information
void print_disk_info(const disk_info_t* disk_info);

// Print safety warnings
void print_safety_warnings(const disk_info_t* disk_info);

// Get user confirmation for dangerous operations
bool get_user_confirmation(const char* operation, const disk_info_t* disk_info);

// Check if running as root
bool is_running_as_root(void);

// Get list of block devices
bool get_block_devices(disk_info_t* devices, int* count, int max_devices);

#endif // DISK_ACCESS_H
