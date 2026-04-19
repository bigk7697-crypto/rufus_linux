/*
 * Rufus Linux - Filesystem Module
 * Copyright © 2026 Port Project
 *
 * Filesystem formatting and management
 */

#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define MAX_FS_NAME 32
#define MAX_LABEL_LENGTH 32
#define MAX_PARTITION_NAME 64
#define SECTOR_SIZE 512
#define MBR_SECTORS 1
#define GPT_HEADER_SECTORS 1
#define GPT_ENTRIES_SECTORS 32

typedef enum {
    FS_UNKNOWN = 0,
    FS_FAT,
    FS_FAT32,
    FS_EXFAT,
    FS_NTFS,
    FS_UDF,
    FS_EXT2,
    FS_EXT3,
    FS_EXT4,
    FS_BTRFS,
    FS_XFS,
    FS_F2FS,
    FS_MAX
} filesystem_type_t;

typedef enum {
    PARTITION_SCHEME_MBR = 0,
    PARTITION_SCHEME_GPT,
    PARTITION_SCHEME_HYBRID,
    PARTITION_SCHEME_MAX
} partition_scheme_t;

typedef struct {
    filesystem_type_t type;
    char name[MAX_FS_NAME];
    char label[MAX_LABEL_LENGTH];
    uint64_t size_bytes;
    uint64_t min_size_bytes;
    uint64_t max_size_bytes;
    bool supports_large_files;
    bool supports_unix_permissions;
    bool supports_journaling;
    bool supports_compression;
    bool supports_encryption;
    bool is_native_linux;
    bool requires_root;
    char mkfs_command[256];
    char mkfs_options[512];
} filesystem_info_t;

typedef struct {
    partition_scheme_t scheme;
    uint64_t start_sector;
    uint64_t end_sector;
    uint64_t size_sectors;
    uint8_t partition_type;
    bool bootable;
    char label[MAX_LABEL_LENGTH];
    filesystem_type_t filesystem;
    char partition_name[MAX_PARTITION_NAME];
} partition_info_t;

typedef struct {
    partition_info_t partitions[16];
    int partition_count;
    partition_scheme_t scheme;
    uint64_t total_sectors;
    bool has_mbr;
    bool has_gpt;
    bool is_hybrid;
} partition_table_t;

// Initialize filesystem module
bool filesystem_init(void);

// Cleanup filesystem module
void filesystem_cleanup(void);

// Get filesystem information
bool get_filesystem_info(filesystem_type_t type, filesystem_info_t* info);

// List supported filesystems
bool list_supported_filesystems(filesystem_info_t* filesystems, int* count, int max_count);

// Check if filesystem is supported
bool is_filesystem_supported(filesystem_type_t type);

// Validate filesystem for device
bool validate_filesystem_for_device(const char* device_path, filesystem_type_t type, uint64_t device_size);

// Format device with filesystem
bool format_device(const char* device_path, filesystem_type_t type, const char* label, 
                   partition_scheme_t scheme, bool quick_format);

// Create partition table
bool create_partition_table(const char* device_path, partition_scheme_t scheme);

// Add partition to table
bool add_partition(const char* device_path, const partition_info_t* partition);

// Read partition table
bool read_partition_table(const char* device_path, partition_table_t* table);

// Write partition table
bool write_partition_table(const char* device_path, const partition_table_t* table);

// Format partition
bool format_partition(const char* device_path, int partition_number, filesystem_type_t type, 
                      const char* label, bool quick_format);

// Get filesystem type from string
filesystem_type_t get_filesystem_type_from_string(const char* fs_name);

// Get filesystem name from type
const char* get_filesystem_name_from_type(filesystem_type_t type);

// Check if filesystem needs mkfs
bool filesystem_needs_mkfs(filesystem_type_t type);

// Execute mkfs command
bool execute_mkfs(const char* device_path, filesystem_type_t type, const char* label, 
                  const char* extra_options);

// Check filesystem after formatting
bool check_filesystem(const char* device_path, filesystem_type_t type);

// Get recommended filesystem for device size
filesystem_type_t get_recommended_filesystem(uint64_t device_size);

// Validate label
bool validate_label(const char* label, filesystem_type_t type);

// Calculate optimal cluster size
uint32_t calculate_optimal_cluster_size(uint64_t device_size, filesystem_type_t type);

// Check if device is already formatted
bool is_device_formatted(const char* device_path);

// Get current filesystem type
filesystem_type_t get_current_filesystem_type(const char* device_path);

// Backup partition table
bool backup_partition_table(const char* device_path, const char* backup_file);

// Restore partition table
bool restore_partition_table(const char* device_path, const char* backup_file);

// Print filesystem information
void print_filesystem_info(const filesystem_info_t* info);

// Print partition table
void print_partition_table(const partition_table_t* table);

#endif // FILESYSTEM_H
