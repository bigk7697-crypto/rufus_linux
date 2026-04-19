/*
 * Rufus Linux - Disk Access Module
 * Copyright © 2026 Port Project
 *
 * Safe disk selection and access for Linux
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mount.h>
#include <linux/fs.h>
#include <linux/hdreg.h>
#include <dirent.h>
#include <errno.h>
#include <pwd.h>
#include <blkid/blkid.h>
#include "disk_access.h"

// Initialize disk access system
bool disk_access_init(void)
{
    // Check if running as root for disk access
    if (!is_running_as_root()) {
        fprintf(stderr, "AVERTISSEMENT: Pas exécuté en tant que root. Certaines opérations pourraient échouer.\n");
    }
    return true;
}

// Cleanup disk access system
void disk_access_cleanup(void)
{
    // Nothing to cleanup for now
}

// Check if running as root
bool is_running_as_root(void)
{
    return (getuid() == 0);
}

// Validate device path (exists, accessible, block device)
bool validate_device_path(const char* device_path)
{
    struct stat st;
    
    if (!device_path || strlen(device_path) == 0) {
        fprintf(stderr, "Erreur: Chemin de périphérique vide\n");
        return false;
    }
    
    if (strlen(device_path) >= MAX_DISK_PATH) {
        fprintf(stderr, "Erreur: Chemin de périphérique trop long\n");
        return false;
    }
    
    if (stat(device_path, &st) != 0) {
        fprintf(stderr, "Erreur: Le périphérique %s n'existe pas\n", device_path);
        return false;
    }
    
    if (!S_ISBLK(st.st_mode)) {
        fprintf(stderr, "Erreur: %s n'est pas un périphérique de bloc\n", device_path);
        return false;
    }
    
    // Check if we can open it for reading
    int fd = open(device_path, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "Erreur: Impossible d'ouvrir %s en lecture: %s\n", 
                device_path, strerror(errno));
        return false;
    }
    close(fd);
    
    return true;
}

// Get disk information from device path
bool get_disk_info(const char* device_path, disk_info_t* disk_info)
{
    int fd;
    struct stat st;
    uint64_t size_bytes;
    
    if (!validate_device_path(device_path) || !disk_info) {
        return false;
    }
    
    memset(disk_info, 0, sizeof(disk_info_t));
    strncpy(disk_info->device_path, device_path, MAX_DISK_PATH - 1);
    
    // Open device
    fd = open(device_path, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "Erreur: Impossible d'ouvrir %s: %s\n", 
                device_path, strerror(errno));
        return false;
    }
    
    // Get device size
    if (ioctl(fd, BLKGETSIZE64, &size_bytes) != 0) {
        fprintf(stderr, "Erreur: Impossible d'obtenir la taille de %s: %s\n", 
                device_path, strerror(errno));
        close(fd);
        return false;
    }
    disk_info->size_bytes = size_bytes;
    disk_info->size_sectors = size_bytes / SECTOR_SIZE;
    
    // Check if removable
    if (ioctl(fd, BLKRRPART, NULL) != 0) {
        // This might fail, but it's not critical
    }
    
    // Try to get model/vendor information from sysfs
    char sysfs_path[512];
    FILE* fp;
    
    // Get model
    snprintf(sysfs_path, sizeof(sysfs_path), "/sys/block/%s/device/model", 
             strrchr(device_path, '/') + 1);
    fp = fopen(sysfs_path, "r");
    if (fp) {
        if (fgets(disk_info->model, sizeof(disk_info->model), fp)) {
            // Remove newline
            char* newline = strchr(disk_info->model, '\n');
            if (newline) *newline = '\0';
        }
        fclose(fp);
    }
    
    // Get vendor
    snprintf(sysfs_path, sizeof(sysfs_path), "/sys/block/%s/device/vendor", 
             strrchr(device_path, '/') + 1);
    fp = fopen(sysfs_path, "r");
    if (fp) {
        if (fgets(disk_info->vendor, sizeof(disk_info->vendor), fp)) {
            char* newline = strchr(disk_info->vendor, '\n');
            if (newline) *newline = '\0';
        }
        fclose(fp);
    }
    
    close(fd);
    
    // Check if it's a USB device
    disk_info->is_usb = (strstr(device_path, "/dev/sd") != NULL) || 
                       (strstr(device_path, "/dev/hd") != NULL);
    
    // Check if removable (simplified check)
    disk_info->is_removable = disk_info->is_usb;
    
    // Check mount points
    disk_info->mount_point_count = 0;
    DIR* mount_dir = opendir("/proc/mounts");
    if (mount_dir) {
        FILE* mounts = fopen("/proc/mounts", "r");
        if (mounts) {
            char line[1024];
            char mount_source[256];
            char mount_target[256];
            char mount_type[64];
            char mount_options[256];
            int mount_freq, mount_passno;
            
            while (fgets(line, sizeof(line), mounts) && 
                   disk_info->mount_point_count < 16) {
                if (sscanf(line, "%255s %255s %63s %255s %d %d",
                          mount_source, mount_target, mount_type, mount_options,
                          &mount_freq, &mount_passno) == 6) {
                    
                    // Check if this mount point is related to our device
                    if (strncmp(mount_source, device_path, strlen(device_path)) == 0) {
                        strncpy(disk_info->mount_points[disk_info->mount_point_count], 
                               mount_target, 255);
                        disk_info->mount_point_count++;
                    }
                }
            }
            fclose(mounts);
        }
        closedir(mount_dir);
    }
    
    // Check partitions
    disk_info->has_partitions = false;
    disk_info->partition_count = 0;
    
    // Look for partition devices
    char base_device[256];
    strncpy(base_device, strrchr(device_path, '/') + 1, sizeof(base_device) - 1);
    
    DIR* dev_dir = opendir("/dev");
    if (dev_dir) {
        struct dirent* entry;
        while ((entry = readdir(dev_dir)) != NULL) {
            if (strncmp(entry->d_name, base_device, strlen(base_device)) == 0 &&
                strlen(entry->d_name) > strlen(base_device)) {
                // This looks like a partition
                disk_info->partition_count++;
                disk_info->has_partitions = true;
            }
        }
        closedir(dev_dir);
    }
    
    // Check if it's likely a system disk (simplified heuristic)
    disk_info->is_system_disk = false;
    if (disk_info->mount_point_count > 0) {
        for (int i = 0; i < disk_info->mount_point_count; i++) {
            if (strcmp(disk_info->mount_points[i], "/") == 0 ||
                strcmp(disk_info->mount_points[i], "/boot") == 0 ||
                strcmp(disk_info->mount_points[i], "/home") == 0) {
                disk_info->is_system_disk = true;
                break;
            }
        }
    }
    
    return true;
}

// Check if device is safe to use (not system disk)
bool is_safe_device(const disk_info_t* disk_info)
{
    if (!disk_info) {
        return false;
    }
    
    // Check size constraints
    if (disk_info->size_bytes < MIN_DISK_SIZE) {
        printf("Périphérique trop petit: %.1f MB (minimum: %.1f MB)\n",
               (double)disk_info->size_bytes / (1024*1024),
               (double)MIN_DISK_SIZE / (1024*1024));
        return false;
    }
    
    if (disk_info->size_bytes > MAX_DISK_SIZE) {
        printf("Périphérique trop grand: %.1f TB (maximum: %.1f TB)\n",
               (double)disk_info->size_bytes / (1024LL*1024*1024*1024),
               (double)MAX_DISK_SIZE / (1024LL*1024*1024*1024));
        return false;
    }
    
    // Check if it's a system disk
    if (disk_info->is_system_disk) {
        printf("AVERTISSEMENT: Ce périphérique contient des partitions système!\n");
        return false;
    }
    
    // Prefer removable/USB devices
    if (!disk_info->is_removable && !disk_info->is_usb) {
        printf("AVERTISSEMENT: Ce n'est pas un périphérique amovible!\n");
        return false;
    }
    
    return true;
}

// Check if device is mounted
bool is_device_mounted(const char* device_path)
{
    FILE* mounts = fopen("/proc/mounts", "r");
    if (!mounts) {
        return false;
    }
    
    char line[1024];
    bool mounted = false;
    
    while (fgets(line, sizeof(line), mounts)) {
        if (strncmp(line, device_path, strlen(device_path)) == 0) {
            mounted = true;
            break;
        }
    }
    
    fclose(mounts);
    return mounted;
}

// Unmount device (all partitions)
bool unmount_device(const char* device_path)
{
    if (!is_running_as_root()) {
        fprintf(stderr, "Erreur: Nécessite les privilèges root pour démonter\n");
        return false;
    }
    
    FILE* mounts = fopen("/proc/mounts", "r");
    if (!mounts) {
        fprintf(stderr, "Erreur: Impossible de lire /proc/mounts\n");
        return false;
    }
    
    char line[1024];
    char mount_source[256];
    char mount_target[256];
    char mount_type[64];
    char mount_options[256];
    int mount_freq, mount_passno;
    bool success = true;
    
    while (fgets(line, sizeof(line), mounts)) {
        if (sscanf(line, "%255s %255s %63s %255s %d %d",
                  mount_source, mount_target, mount_type, mount_options,
                  &mount_freq, &mount_passno) == 6) {
            
            if (strncmp(mount_source, device_path, strlen(device_path)) == 0) {
                printf("Démontage de %s (monté sur %s)\n", mount_source, mount_target);
                if (umount(mount_target) != 0) {
                    fprintf(stderr, "Erreur: Impossible de démonter %s: %s\n", 
                            mount_target, strerror(errno));
                    success = false;
                } else {
                    printf("  -> Succès\n");
                }
            }
        }
    }
    
    fclose(mounts);
    return success;
}

// Create MBR backup before writing
bool create_mbr_backup(const char* device_path, mbr_backup_t* backup)
{
    int fd;
    ssize_t bytes_read;
    
    if (!backup || !device_path) {
        return false;
    }
    
    memset(backup, 0, sizeof(mbr_backup_t));
    
    fd = open(device_path, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "Erreur: Impossible d'ouvrir %s pour backup MBR: %s\n", 
                device_path, strerror(errno));
        return false;
    }
    
    bytes_read = read(fd, backup->data, MBR_BACKUP_SIZE);
    if (bytes_read < MBR_SIZE) {
        fprintf(stderr, "Erreur: Lecture MBR incomplète: %zd bytes lus\n", bytes_read);
        close(fd);
        return false;
    }
    
    backup->size = bytes_read;
    backup->valid = true;
    
    close(fd);
    printf("Backup MBR créé: %zd bytes\n", bytes_read);
    
    return true;
}

// Restore MBR from backup
bool restore_mbr_backup(const char* device_path, const mbr_backup_t* backup)
{
    int fd;
    ssize_t bytes_written;
    
    if (!backup || !backup->valid || !device_path) {
        return false;
    }
    
    fd = open(device_path, O_WRONLY);
    if (fd < 0) {
        fprintf(stderr, "Erreur: Impossible d'ouvrir %s pour restauration MBR: %s\n", 
                device_path, strerror(errno));
        return false;
    }
    
    bytes_written = write(fd, backup->data, backup->size);
    if (bytes_written != backup->size) {
        fprintf(stderr, "Erreur: Restauration MBR incomplète: %zd/%zd bytes écrits\n", 
                bytes_written, backup->size);
        close(fd);
        return false;
    }
    
    // Sync to ensure data is written
    fsync(fd);
    close(fd);
    
    printf("MBR restauré: %zd bytes\n", bytes_written);
    return true;
}

// Save MBR backup to file
bool save_mbr_backup_to_file(const mbr_backup_t* backup, const char* filename)
{
    FILE* fp;
    
    if (!backup || !backup->valid || !filename) {
        return false;
    }
    
    fp = fopen(filename, "wb");
    if (!fp) {
        fprintf(stderr, "Erreur: Impossible de créer le fichier de backup %s: %s\n", 
                filename, strerror(errno));
        return false;
    }
    
    if (fwrite(backup->data, 1, backup->size, fp) != backup->size) {
        fprintf(stderr, "Erreur: Écriture backup incomplète\n");
        fclose(fp);
        return false;
    }
    
    fclose(fp);
    printf("Backup MBR sauvegardé dans %s\n", filename);
    return true;
}

// Load MBR backup from file
bool load_mbr_backup_from_file(const char* filename, mbr_backup_t* backup)
{
    FILE* fp;
    long file_size;
    
    if (!backup || !filename) {
        return false;
    }
    
    memset(backup, 0, sizeof(mbr_backup_t));
    
    fp = fopen(filename, "rb");
    if (!fp) {
        fprintf(stderr, "Erreur: Impossible d'ouvrir le fichier de backup %s: %s\n", 
                filename, strerror(errno));
        return false;
    }
    
    fseek(fp, 0, SEEK_END);
    file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    if (file_size > MBR_BACKUP_SIZE) {
        fprintf(stderr, "Erreur: Fichier de backup trop grand: %ld bytes\n", file_size);
        fclose(fp);
        return false;
    }
    
    if (fread(backup->data, 1, file_size, fp) != file_size) {
        fprintf(stderr, "Erreur: Lecture backup incomplète\n");
        fclose(fp);
        return false;
    }
    
    backup->size = file_size;
    backup->valid = true;
    
    fclose(fp);
    printf("Backup MBR chargé depuis %s: %ld bytes\n", filename, file_size);
    return true;
}

// Print disk information
void print_disk_info(const disk_info_t* disk_info)
{
    if (!disk_info) {
        printf("Erreur: Informations de périphérique invalides\n");
        return;
    }
    
    printf("=== Informations du Périphérique ===\n");
    printf("Chemin: %s\n", disk_info->device_path);
    printf("Taille: %.1f GB (%llu secteurs)\n", 
           (double)disk_info->size_bytes / (1024.0*1024.0*1024.0),
           (unsigned long long)disk_info->size_sectors);
    printf("Modèle: %s\n", disk_info->model[0] ? disk_info->model : "Inconnu");
    printf("Fabricant: %s\n", disk_info->vendor[0] ? disk_info->vendor : "Inconnu");
    printf("USB: %s\n", disk_info->is_usb ? "Oui" : "Non");
    printf("Amovible: %s\n", disk_info->is_removable ? "Oui" : "Non");
    printf("Partitions: %d\n", disk_info->partition_count);
    printf("Points de montage: %d\n", disk_info->mount_point_count);
    
    if (disk_info->mount_point_count > 0) {
        printf("  Monté sur:\n");
        for (int i = 0; i < disk_info->mount_point_count; i++) {
            printf("    %s\n", disk_info->mount_points[i]);
        }
    }
    
    printf("Système: %s\n", disk_info->is_system_disk ? "Oui (DANGER!)" : "Non");
}

// Print safety warnings
void print_safety_warnings(const disk_info_t* disk_info)
{
    if (!disk_info) {
        return;
    }
    
    printf("\n=== AVERTISSEMENTS DE SÉCURITÉ ===\n");
    
    if (disk_info->is_system_disk) {
        printf("DANGER: Ce périphérique contient des partitions système!\n");
        printf("L'utiliser détruira votre système d'exploitation!\n");
    }
    
    if (disk_info->mount_point_count > 0) {
        printf("AVERTISSEMENT: Ce périphérique est actuellement monté!\n");
        printf("Les données pourraient être corrompues si utilisé.\n");
    }
    
    if (!disk_info->is_removable && !disk_info->is_usb) {
        printf("AVERTISSEMENT: Ce n'est pas un périphérique amovible!\n");
        printf("Vérifiez que c'est bien la bonne cible.\n");
    }
    
    if (disk_info->size_bytes > 64ULL * 1024 * 1024 * 1024 * 1024) { // 64GB
        printf("AVERTISSEMENT: Périphérique très grand (%.1f GB)!\n", 
               (double)disk_info->size_bytes / (1024.0*1024.0*1024.0));
        printf("Vérifiez que ce n'est pas un disque dur interne.\n");
    }
    
    printf("Toutes les données sur ce périphérique seront PERDUES!\n");
}

// Get user confirmation for dangerous operations
bool get_user_confirmation(const char* operation, const disk_info_t* disk_info)
{
    char input[256];
    
    printf("\n=== CONFIRMATION REQUISE ===\n");
    printf("Opération: %s\n", operation);
    print_disk_info(disk_info);
    print_safety_warnings(disk_info);
    
    printf("\nPour continuer, tapez 'oui' (exactement): ");
    fflush(stdout);
    
    if (fgets(input, sizeof(input), stdin)) {
        // Remove newline
        char* newline = strchr(input, '\n');
        if (newline) *newline = '\0';
        
        if (strcmp(input, "oui") == 0) {
            printf("Opération confirmée.\n");
            return true;
        }
    }
    
    printf("Opération ANNULÉE.\n");
    return false;
}

// Get list of block devices
bool get_block_devices(disk_info_t* devices, int* count, int max_devices)
{
    DIR* dev_dir;
    struct dirent* entry;
    char device_path[512];
    int device_count = 0;
    
    if (!devices || !count || max_devices <= 0) {
        return false;
    }
    
    *count = 0;
    
    dev_dir = opendir("/dev");
    if (!dev_dir) {
        fprintf(stderr, "Erreur: Impossible d'ouvrir /dev\n");
        return false;
    }
    
    while ((entry = readdir(dev_dir)) != NULL && device_count < max_devices) {
        // Look for sd*, hd*, nvme*, vd* devices
        if ((strncmp(entry->d_name, "sd", 2) == 0 && strlen(entry->d_name) <= 3) ||
            (strncmp(entry->d_name, "hd", 2) == 0 && strlen(entry->d_name) <= 3) ||
            (strncmp(entry->d_name, "nvme", 4) == 0 && strlen(entry->d_name) <= 7) ||
            (strncmp(entry->d_name, "vd", 2) == 0 && strlen(entry->d_name) <= 3)) {
            
            snprintf(device_path, sizeof(device_path), "/dev/%s", entry->d_name);
            
            if (get_disk_info(device_path, &devices[device_count])) {
                device_count++;
            }
        }
    }
    
    closedir(dev_dir);
    *count = device_count;
    
    return true;
}
