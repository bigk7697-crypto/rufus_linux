/*
 * Rufus Linux - Filesystem Module
 * Copyright © 2026 Port Project
 *
 * Filesystem formatting and management
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <blkid/blkid.h>
#include "filesystem.h"
#include "disk_access.h"
#include "security.h"

// Filesystem database
static const filesystem_info_t filesystem_database[FS_MAX] = {
    // FAT
    {
        .type = FS_FAT,
        .name = "FAT",
        .label = "",
        .size_bytes = 0,
        .min_size_bytes = 512 * 1024,        // 512KB
        .max_size_bytes = 2ULL * 1024 * 1024 * 1024, // 2GB
        .supports_large_files = false,
        .supports_unix_permissions = false,
        .supports_journaling = false,
        .supports_compression = false,
        .supports_encryption = false,
        .is_native_linux = false,
        .requires_root = true,
        .mkfs_command = "mkfs.fat",
        .mkfs_options = "-F 12 -n"
    },
    // FAT32
    {
        .type = FS_FAT32,
        .name = "FAT32",
        .label = "",
        .size_bytes = 0,
        .min_size_bytes = 32 * 1024 * 1024,     // 32MB
        .max_size_bytes = 2ULL * 1024 * 1024 * 1024, // 2TB (theoretical)
        .supports_large_files = false,
        .supports_unix_permissions = false,
        .supports_journaling = false,
        .supports_compression = false,
        .supports_encryption = false,
        .is_native_linux = false,
        .requires_root = true,
        .mkfs_command = "mkfs.fat",
        .mkfs_options = "-F 32 -n"
    },
    // exFAT
    {
        .type = FS_EXFAT,
        .name = "exFAT",
        .label = "",
        .size_bytes = 0,
        .min_size_bytes = 512 * 1024,        // 512KB
        .max_size_bytes = 128ULL * 1024 * 1024 * 1024 * 1024, // 128PB
        .supports_large_files = true,
        .supports_unix_permissions = false,
        .supports_journaling = false,
        .supports_compression = false,
        .supports_encryption = false,
        .is_native_linux = false,
        .requires_root = true,
        .mkfs_command = "mkfs.exfat",
        .mkfs_options = "-n"
    },
    // NTFS
    {
        .type = FS_NTFS,
        .name = "NTFS",
        .label = "",
        .size_bytes = 0,
        .min_size_bytes = 1 * 1024 * 1024,     // 1MB
        .max_size_bytes = 256ULL * 1024 * 1024 * 1024 * 1024, // 256PB
        .supports_large_files = true,
        .supports_unix_permissions = false,
        .supports_journaling = true,
        .supports_compression = true,
        .supports_encryption = true,
        .is_native_linux = false,
        .requires_root = true,
        .mkfs_command = "mkfs.ntfs",
        .mkfs_options = "-f -L"
    },
    // UDF
    {
        .type = FS_UDF,
        .name = "UDF",
        .label = "",
        .size_bytes = 0,
        .min_size_bytes = 16 * 1024 * 1024,    // 16MB
        .max_size_bytes = 16ULL * 1024 * 1024 * 1024 * 1024, // 16PB
        .supports_large_files = true,
        .supports_unix_permissions = false,
        .supports_journaling = false,
        .supports_compression = false,
        .supports_encryption = false,
        .is_native_linux = false,
        .requires_root = true,
        .mkfs_command = "mkudffs",
        .mkfs_options = "--media-type=hd --blocksize=512"
    },
    // ext2
    {
        .type = FS_EXT2,
        .name = "ext2",
        .label = "",
        .size_bytes = 0,
        .min_size_bytes = 16 * 1024 * 1024,    // 16MB
        .max_size_bytes = 32ULL * 1024 * 1024 * 1024, // 32TB
        .supports_large_files = false,
        .supports_unix_permissions = true,
        .supports_journaling = false,
        .supports_compression = false,
        .supports_encryption = false,
        .is_native_linux = true,
        .requires_root = true,
        .mkfs_command = "mkfs.ext2",
        .mkfs_options = "-L"
    },
    // ext3
    {
        .type = FS_EXT3,
        .name = "ext3",
        .label = "",
        .size_bytes = 0,
        .min_size_bytes = 16 * 1024 * 1024,    // 16MB
        .max_size_bits = 32ULL * 1024 * 1024 * 1024, // 32TB
        .supports_large_files = true,
        .supports_unix_permissions = true,
        .supports_journaling = true,
        .supports_compression = false,
        .supports_encryption = false,
        .is_native_linux = true,
        .requires_root = true,
        .mkfs_command = "mkfs.ext3",
        .mkfs_options = "-L"
    },
    // ext4
    {
        .type = FS_EXT4,
        .name = "ext4",
        .label = "",
        .size_bytes = 0,
        .min_size_bytes = 16 * 1024 * 1024,    // 16MB
        .max_size_bytes = 1ULL * 1024 * 1024 * 1024 * 1024, // 1EB
        .supports_large_files = true,
        .supports_unix_permissions = true,
        .supports_journaling = true,
        .supports_compression = false,
        .supports_encryption = false,
        .is_native_linux = true,
        .requires_root = true,
        .mkfs_command = "mkfs.ext4",
        .mkfs_options = "-L"
    },
    // Btrfs
    {
        .type = FS_BTRFS,
        .name = "Btrfs",
        .label = "",
        .size_bytes = 0,
        .min_size_bytes = 256 * 1024 * 1024,   // 256MB
        .max_size_bytes = 16ULL * 1024 * 1024 * 1024 * 1024, // 16EB
        .supports_large_files = true,
        .supports_unix_permissions = true,
        .supports_journaling = true,
        .supports_compression = true,
        .supports_encryption = true,
        .is_native_linux = true,
        .requires_root = true,
        .mkfs_command = "mkfs.btrfs",
        .mkfs_options = "-L"
    },
    // XFS
    {
        .type = FS_XFS,
        .name = "XFS",
        .label = "",
        .size_bytes = 0,
        .min_size_bytes = 300 * 1024 * 1024,   // 300MB
        .max_size_bytes = 8ULL * 1024 * 1024 * 1024 * 1024, // 8EB
        .supports_large_files = true,
        .supports_unix_permissions = true,
        .supports_journaling = true,
        .supports_compression = false,
        .supports_encryption = false,
        .is_native_linux = true,
        .requires_root = true,
        .mkfs_command = "mkfs.xfs",
        .mkfs_options = "-L"
    },
    // F2FS
    {
        .type = FS_F2FS,
        .name = "F2FS",
        .label = "",
        .size_bytes = 0,
        .min_size_bytes = 100 * 1024 * 1024,   // 100MB
        .max_size_bytes = 16ULL * 1024 * 1024 * 1024 * 1024, // 16EB
        .supports_large_files = true,
        .supports_unix_permissions = true,
        .supports_journaling = false,
        .supports_compression = true,
        .supports_encryption = false,
        .is_native_linux = true,
        .requires_root = true,
        .mkfs_command = "mkfs.f2fs",
        .mkfs_options = "-l"
    }
};

// Initialize filesystem module
bool filesystem_init(void)
{
    log_info("Filesystem module initialized");
    return true;
}

// Cleanup filesystem module
void filesystem_cleanup(void)
{
    log_info("Filesystem module cleaned up");
}

// Get filesystem information
bool get_filesystem_info(filesystem_type_t type, filesystem_info_t* info)
{
    if (type >= FS_MAX || type <= FS_UNKNOWN || !info) {
        return false;
    }
    
    *info = filesystem_database[type];
    return true;
}

// List supported filesystems
bool list_supported_filesystems(filesystem_info_t* filesystems, int* count, int max_count)
{
    if (!filesystems || !count || max_count <= 0) {
        return false;
    }
    
    int supported = 0;
    
    for (int i = 1; i < FS_MAX; i++) {
        // Check if mkfs command is available
        char command[512];
        snprintf(command, sizeof(command), "which %s >/dev/null 2>&1", filesystem_database[i].mkfs_command);
        
        if (system(command) == 0) {
            if (supported < max_count) {
                filesystems[supported] = filesystem_database[i];
                supported++;
            }
        }
    }
    
    *count = supported;
    return supported > 0;
}

// Check if filesystem is supported
bool is_filesystem_supported(filesystem_type_t type)
{
    if (type >= FS_MAX || type <= FS_UNKNOWN) {
        return false;
    }
    
    char command[512];
    snprintf(command, sizeof(command), "which %s >/dev/null 2>&1", filesystem_database[type].mkfs_command);
    
    return (system(command) == 0);
}

// Validate filesystem for device
bool validate_filesystem_for_device(const char* device_path, filesystem_type_t type, uint64_t device_size)
{
    filesystem_info_t fs_info;
    
    if (!get_filesystem_info(type, &fs_info)) {
        log_error("Invalid filesystem type: %d", type);
        return false;
    }
    
    if (!is_filesystem_supported(type)) {
        log_error("Filesystem %s not supported (mkfs command not found)", fs_info.name);
        return false;
    }
    
    if (device_size < fs_info.min_size_bytes) {
        log_error("Device too small for %s: %llu bytes (minimum: %llu bytes)",
                 fs_info.name, (unsigned long long)device_size, (unsigned long long)fs_info.min_size_bytes);
        return false;
    }
    
    if (device_size > fs_info.max_size_bytes) {
        log_error("Device too large for %s: %llu bytes (maximum: %llu bytes)",
                 fs_info.name, (unsigned long long)device_size, (unsigned long long)fs_info.max_size_bytes);
        return false;
    }
    
    return true;
}

// Execute mkfs command
bool execute_mkfs(const char* device_path, filesystem_type_t type, const char* label, 
                  const char* extra_options)
{
    filesystem_info_t fs_info;
    char command[1024];
    int result;
    
    if (!get_filesystem_info(type, &fs_info)) {
        return false;
    }
    
    // Build mkfs command
    snprintf(command, sizeof(command), "%s %s", fs_info.mkfs_command, fs_info.mkfs_options);
    
    // Add label if provided
    if (label && strlen(label) > 0) {
        char label_option[256];
        snprintf(label_option, sizeof(label_option), " \"%s\"", label);
        strcat(command, label_option);
    }
    
    // Add extra options if provided
    if (extra_options) {
        strcat(command, " ");
        strcat(command, extra_options);
    }
    
    // Add device path
    strcat(command, " ");
    strcat(command, device_path);
    
    log_info("Executing mkfs command: %s", command);
    
    // Execute command
    result = system(command);
    
    if (result == 0) {
        log_info("mkfs completed successfully for %s on %s", fs_info.name, device_path);
        return true;
    } else {
        log_error("mkfs failed for %s on %s (exit code: %d)", fs_info.name, device_path, result);
        return false;
    }
}

// Format device with filesystem
bool format_device(const char* device_path, filesystem_type_t type, const char* label, 
                   partition_scheme_t scheme, bool quick_format)
{
    disk_info_t disk_info;
    
    if (!device_path) {
        log_error("Device path is NULL");
        return false;
    }
    
    // Get device information
    if (!get_disk_info(device_path, &disk_info)) {
        log_error("Failed to get device information for %s", device_path);
        return false;
    }
    
    // Validate filesystem for device
    if (!validate_filesystem_for_device(device_path, type, disk_info.size_bytes)) {
        return false;
    }
    
    log_info("Formatting device %s with %s (size: %.1f GB)", 
             device_path, get_filesystem_name_from_type(type),
             (double)disk_info.size_bytes / (1024*1024*1024));
    
    // Unmount device if mounted
    if (is_device_mounted(device_path)) {
        log_info("Unmounting device %s before formatting", device_path);
        if (!unmount_device(device_path)) {
            log_warning("Failed to unmount device %s", device_path);
        }
    }
    
    // Create partition table if needed
    if (scheme != PARTITION_SCHEME_MAX) {
        partition_table_t table = {0};
        table.scheme = scheme;
        table.total_sectors = disk_info.size_sectors;
        
        // Add single partition
        table.partitions[0].start_sector = 2048; // Start after MBR/GPT
        table.partitions[0].end_sector = disk_info.size_sectors - 1;
        table.partitions[0].size_sectors = table.partitions[0].end_sector - table.partitions[0].start_sector + 1;
        table.partitions[0].bootable = true;
        table.partitions[0].filesystem = type;
        if (label) {
            strncpy(table.partitions[0].label, label, MAX_LABEL_LENGTH - 1);
            table.partitions[0].label[MAX_LABEL_LENGTH - 1] = '\0';
        }
        table.partition_count = 1;
        
        if (!write_partition_table(device_path, &table)) {
            log_error("Failed to create partition table on %s", device_path);
            return false;
        }
        
        // Use partition 1 for formatting
        char partition_path[256];
        snprintf(partition_path, sizeof(partition_path), "%s1", device_path);
        
        return execute_mkfs(partition_path, type, label, quick_format ? "-F" : "");
    } else {
        // Format entire device
        return execute_mkfs(device_path, type, label, quick_format ? "-F" : "");
    }
}

// Create partition table
bool create_partition_table(const char* device_path, partition_scheme_t scheme)
{
    // This is a simplified implementation
    // In a real implementation, you would use fdisk/parted/sfdisk
    
    char command[512];
    int result;
    
    switch (scheme) {
        case PARTITION_SCHEME_MBR:
            // Use sfdisk to create MBR partition table
            snprintf(command, sizeof(command), "echo 'label: dos' | sfdisk %s", device_path);
            break;
            
        case PARTITION_SCHEME_GPT:
            // Use sfdisk to create GPT partition table
            snprintf(command, sizeof(command), "echo 'label: gpt' | sfdisk %s", device_path);
            break;
            
        default:
            log_error("Unsupported partition scheme: %d", scheme);
            return false;
    }
    
    log_info("Creating partition table: %s", command);
    result = system(command);
    
    if (result == 0) {
        log_info("Partition table created successfully on %s", device_path);
        return true;
    } else {
        log_error("Failed to create partition table on %s (exit code: %d)", device_path, result);
        return false;
    }
}

// Write partition table (simplified)
bool write_partition_table(const char* device_path, const partition_table_t* table)
{
    if (!device_path || !table) {
        return false;
    }
    
    log_info("Writing partition table to %s", device_path);
    
    // Create partition table first
    if (!create_partition_table(device_path, table->scheme)) {
        return false;
    }
    
    // Add partitions (simplified - would use sfdisk script in real implementation)
    for (int i = 0; i < table->partition_count; i++) {
        const partition_info_t* partition = &table->partitions[i];
        
        char command[1024];
        snprintf(command, sizeof(command), 
                "echo '%llu,%llu,%d' | sfdisk %s",
                (unsigned long long)partition->start_sector,
                (unsigned long long)partition->size_sectors,
                partition->bootable ? 0x80 : 0x83,
                device_path);
        
        int result = system(command);
        if (result != 0) {
            log_error("Failed to add partition %d to %s", i + 1, device_path);
            return false;
        }
    }
    
    // Inform kernel of partition table changes
    snprintf(command, sizeof(command), "partprobe %s", device_path);
    system(command);
    
    log_info("Partition table written successfully to %s", device_path);
    return true;
}

// Get filesystem type from string
filesystem_type_t get_filesystem_type_from_string(const char* fs_name)
{
    if (!fs_name) {
        return FS_UNKNOWN;
    }
    
    for (int i = 1; i < FS_MAX; i++) {
        if (strcasecmp(fs_name, filesystem_database[i].name) == 0) {
            return (filesystem_type_t)i;
        }
    }
    
    return FS_UNKNOWN;
}

// Get filesystem name from type
const char* get_filesystem_name_from_type(filesystem_type_t type)
{
    if (type >= FS_MAX || type <= FS_UNKNOWN) {
        return "Unknown";
    }
    
    return filesystem_database[type].name;
}

// Check if filesystem needs mkfs
bool filesystem_needs_mkfs(filesystem_type_t type)
{
    filesystem_info_t info;
    return get_filesystem_info(type, &info) && strlen(info.mkfs_command) > 0;
}

// Check filesystem after formatting
bool check_filesystem(const char* device_path, filesystem_type_t type)
{
    char command[512];
    int result;
    
    // Use fsck to check filesystem
    switch (type) {
        case FS_EXT2:
        case FS_EXT3:
        case FS_EXT4:
            snprintf(command, sizeof(command), "fsck.ext4 -n %s", device_path);
            break;
            
        case FS_XFS:
            snprintf(command, sizeof(command), "xfs_repair -n %s", device_path);
            break;
            
        case FS_BTRFS:
            snprintf(command, sizeof(command), "btrfs check %s", device_path);
            break;
            
        default:
            log_info("No filesystem check available for %s", get_filesystem_name_from_type(type));
            return true; // Not an error, just no check available
    }
    
    log_info("Checking filesystem: %s", command);
    result = system(command);
    
    if (result == 0) {
        log_info("Filesystem check passed for %s", device_path);
        return true;
    } else {
        log_warning("Filesystem check failed for %s (exit code: %d)", device_path, result);
        return false;
    }
}

// Get recommended filesystem for device size
filesystem_type_t get_recommended_filesystem(uint64_t device_size)
{
    // For small devices (< 2GB), use FAT32
    if (device_size < 2ULL * 1024 * 1024 * 1024) {
        return FS_FAT32;
    }
    
    // For medium devices (2GB - 32GB), use exFAT
    if (device_size < 32ULL * 1024 * 1024 * 1024) {
        return FS_EXFAT;
    }
    
    // For large devices, use ext4 (Linux native)
    return FS_EXT4;
}

// Validate label
bool validate_label(const char* label, filesystem_type_t type)
{
    if (!label) {
        return false;
    }
    
    size_t len = strlen(label);
    
    // Check length
    if (len == 0 || len > MAX_LABEL_LENGTH) {
        return false;
    }
    
    // Check for invalid characters
    for (size_t i = 0; i < len; i++) {
        char c = label[i];
        
        // Allow alphanumeric, space, hyphen, underscore
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
              c == ' ' || c == '-' || c == '_')) {
            return false;
        }
    }
    
    return true;
}

// Calculate optimal cluster size
uint32_t calculate_optimal_cluster_size(uint64_t device_size, filesystem_type_t type)
{
    switch (type) {
        case FS_FAT32:
            // For FAT32, use 4KB clusters for most sizes
            return 4096;
            
        case FS_EXFAT:
            // For exFAT, use 4KB to 128KB clusters based on size
            if (device_size < 1ULL * 1024 * 1024 * 1024) { // < 1GB
                return 4096;
            } else if (device_size < 32ULL * 1024 * 1024 * 1024) { // < 32GB
                return 32768;
            } else {
                return 131072; // 128KB
            }
            
        case FS_EXT4:
            // For ext4, use 4KB block size (standard)
            return 4096;
            
        default:
            return 4096; // Default to 4KB
    }
}

// Check if device is already formatted
bool is_device_formatted(const char* device_path)
{
    blkid_probe pr = blkid_new_probe_from_filename(device_path);
    if (!pr) {
        return false;
    }
    
    int rc = blkid_do_probe(pr);
    bool formatted = (rc == 0);
    
    blkid_free_probe(pr);
    return formatted;
}

// Get current filesystem type
filesystem_type_t get_current_filesystem_type(const char* device_path)
{
    blkid_probe pr = blkid_new_probe_from_filename(device_path);
    if (!pr) {
        return FS_UNKNOWN;
    }
    
    int rc = blkid_do_probe(pr);
    if (rc != 0) {
        blkid_free_probe(pr);
        return FS_UNKNOWN;
    }
    
    const char* fstype = blkid_probe_get_value(pr, "TYPE");
    filesystem_type_t type = FS_UNKNOWN;
    
    if (fstype) {
        type = get_filesystem_type_from_string(fstype);
    }
    
    blkid_free_probe(pr);
    return type;
}

// Print filesystem information
void print_filesystem_info(const filesystem_info_t* info)
{
    if (!info) {
        return;
    }
    
    printf("=== Filesystem Information ===\n");
    printf("Type: %s\n", info->name);
    printf("Min Size: %.1f MB\n", (double)info->min_size_bytes / (1024*1024));
    printf("Max Size: %.1f %s\n", 
           info->max_size_bytes >= (1024ULL*1024*1024*1024) ? 
           (double)info->max_size_bytes / (1024LL*1024*1024*1024) : 
           (double)info->max_size_bytes / (1024*1024*1024),
           info->max_size_bytes >= (1024ULL*1024*1024*1024) ? "TB" : "GB");
    printf("Large Files: %s\n", info->supports_large_files ? "Yes" : "No");
    printf("Unix Permissions: %s\n", info->supports_unix_permissions ? "Yes" : "No");
    printf("Journaling: %s\n", info->supports_journaling ? "Yes" : "No");
    printf("Compression: %s\n", info->supports_compression ? "Yes" : "No");
    printf("Encryption: %s\n", info->supports_encryption ? "Yes" : "No");
    printf("Linux Native: %s\n", info->is_native_linux ? "Yes" : "No");
    printf("Requires Root: %s\n", info->requires_root ? "Yes" : "No");
    printf("mkfs Command: %s\n", info->mkfs_command);
    printf("mkfs Options: %s\n", info->mkfs_options);
}

// Print partition table
void print_partition_table(const partition_table_t* table)
{
    if (!table) {
        return;
    }
    
    printf("=== Partition Table ===\n");
    printf("Scheme: %s\n", 
           table->scheme == PARTITION_SCHEME_MBR ? "MBR" :
           table->scheme == PARTITION_SCHEME_GPT ? "GPT" : "Hybrid");
    printf("Total Sectors: %llu\n", (unsigned long long)table->total_sectors);
    printf("Partitions: %d\n\n", table->partition_count);
    
    for (int i = 0; i < table->partition_count; i++) {
        const partition_info_t* partition = &table->partitions[i];
        
        printf("Partition %d:\n", i + 1);
        printf("  Start Sector: %llu\n", (unsigned long long)partition->start_sector);
        printf("  End Sector: %llu\n", (unsigned long long)partition->end_sector);
        printf("  Size: %.1f GB\n", (double)partition->size_sectors * SECTOR_SIZE / (1024*1024*1024));
        printf("  Bootable: %s\n", partition->bootable ? "Yes" : "No");
        printf("  Filesystem: %s\n", get_filesystem_name_from_type(partition->filesystem));
        if (strlen(partition->label) > 0) {
            printf("  Label: %s\n", partition->label);
        }
        printf("\n");
    }
}
