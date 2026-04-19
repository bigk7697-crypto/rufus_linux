/*
 * Rufus Linux - ISO Writing Module
 * Copyright © 2026 Port Project
 *
 * ISO image writing to USB devices
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <pthread.h>
#include "iso_write.h"
#include "disk_access.h"

// Global cancellation flag
static volatile bool write_cancelled = false;
static pthread_mutex_t cancel_mutex = PTHREAD_MUTEX_INITIALIZER;

// Initialize ISO writing system
bool iso_write_init(void)
{
    write_cancelled = false;
    return true;
}

// Cleanup ISO writing system
void iso_write_cleanup(void)
{
    pthread_mutex_destroy(&cancel_mutex);
}

// Cancel ongoing write operation (thread-safe)
void cancel_write_operation(void)
{
    pthread_mutex_lock(&cancel_mutex);
    write_cancelled = true;
    pthread_mutex_unlock(&cancel_mutex);
}

// Check if write operation was cancelled
bool is_write_cancelled(void)
{
    bool cancelled;
    pthread_mutex_lock(&cancel_mutex);
    cancelled = write_cancelled;
    pthread_mutex_unlock(&cancel_mutex);
    return cancelled;
}

// Get ISO file information
bool get_iso_info(const char* iso_path, iso_info_t* iso_info)
{
    struct stat st;
    FILE* fp;
    char buffer[2048];
    
    if (!iso_path || !iso_info) {
        return false;
    }
    
    memset(iso_info, 0, sizeof(iso_info_t));
    strncpy(iso_info->filepath, iso_path, MAX_ISO_PATH - 1);
    
    // Check if file exists and get size
    if (stat(iso_path, &st) != 0) {
        fprintf(stderr, "Erreur: Impossible d'accéder au fichier ISO %s: %s\n", 
                iso_path, strerror(errno));
        return false;
    }
    
    if (!S_ISREG(st.st_mode)) {
        fprintf(stderr, "Erreur: %s n'est pas un fichier régulier\n", iso_path);
        return false;
    }
    
    iso_info->size_bytes = st.st_size;
    iso_info->size_sectors = (st.st_size + 511) / 512;  // Round up to sectors
    
    // Try to read ISO 9660 primary volume descriptor
    fp = fopen(iso_path, "rb");
    if (!fp) {
        fprintf(stderr, "Erreur: Impossible d'ouvrir le fichier ISO %s: %s\n", 
                iso_path, strerror(errno));
        return false;
    }
    
    // ISO 9660 primary volume descriptor is at sector 16 (offset 16 * 2048 = 32768)
    if (fseek(fp, 32768, SEEK_SET) != 0) {
        fprintf(stderr, "Erreur: Impossible de lire le descripteur ISO\n");
        fclose(fp);
        return false;
    }
    
    if (fread(buffer, 1, 2048, fp) == 2048) {
        // Check for ISO 9660 primary volume descriptor
        if (buffer[0] == 0x01 && strncmp(buffer + 1, "CD001", 5) == 0) {
            iso_info->is_valid = true;
            
            // Extract volume identifier (bytes 40-71)
            strncpy(iso_info->volume_id, buffer + 40, 32);
            iso_info->volume_id[32] = '\0';
            
            // Remove trailing spaces
            char* end = iso_info->volume_id + strlen(iso_info->volume_id) - 1;
            while (end >= iso_info->volume_id && *end == ' ') {
                *end = '\0';
                end--;
            }
            
            // Extract publisher (bytes 318-447)
            strncpy(iso_info->publisher, buffer + 318, 128);
            iso_info->publisher[128] = '\0';
            
            // Remove trailing spaces
            end = iso_info->publisher + strlen(iso_info->publisher) - 1;
            while (end >= iso_info->publisher && *end == ' ') {
                *end = '\0';
                end--;
            }
            
            // Extract application (bytes 574-703)
            strncpy(iso_info->application, buffer + 574, 128);
            iso_info->application[128] = '\0';
            
            // Remove trailing spaces
            end = iso_info->application + strlen(iso_info->application) - 1;
            while (end >= iso_info->application && *end == ' ') {
                *end = '\0';
                end--;
            }
            
            // Check if bootable (look for El Torito boot catalog)
            // This is a simplified check - real implementation would be more complex
            iso_info->is_bootable = true;  // Assume bootable for now
        }
    }
    
    fclose(fp);
    
    if (!iso_info->is_valid) {
        fprintf(stderr, "Erreur: %s n'est pas une image ISO 9660 valide\n", iso_path);
        return false;
    }
    
    return true;
}

// Validate ISO file
bool validate_iso_file(const char* iso_path)
{
    iso_info_t iso_info;
    return get_iso_info(iso_path, &iso_info);
}

// Check if device has enough space for ISO
bool check_device_space(const char* device_path, const iso_info_t* iso_info)
{
    disk_info_t disk_info;
    
    if (!device_path || !iso_info) {
        return false;
    }
    
    if (!get_disk_info(device_path, &disk_info)) {
        return false;
    }
    
    if (disk_info.size_bytes < iso_info->size_bytes) {
        fprintf(stderr, "Erreur: Espace insuffisant\n");
        fprintf(stderr, "  Taille ISO: %.1f GB\n", (double)iso_info->size_bytes / (1024*1024*1024));
        fprintf(stderr, "  Taille périphérique: %.1f GB\n", (double)disk_info.size_bytes / (1024*1024*1024));
        return false;
    }
    
    return true;
}

// Create progress bar string
void create_progress_bar(float percent, char* buffer, size_t buffer_size)
{
    int filled = (int)(percent * MAX_PROGRESS_MARKERS / 100.0f);
    int empty = MAX_PROGRESS_MARKERS - filled;
    
    if (buffer_size < MAX_PROGRESS_MARKERS + 1) {
        return;
    }
    
    memset(buffer, '=', filled);
    memset(buffer + filled, '-', empty);
    buffer[MAX_PROGRESS_MARKERS] = '\0';
}

// Update progress display
void update_progress(write_progress_t* progress, uint64_t bytes_written, uint64_t total_bytes)
{
    if (!progress) {
        return;
    }
    
    progress->bytes_written = bytes_written;
    progress->total_bytes = total_bytes;
    progress->percent_complete = (float)bytes_written * 100.0f / total_bytes;
    
    create_progress_bar(progress->percent_complete, progress->progress_bar, 
                      sizeof(progress->progress_bar));
    
    // Calculate ETA (simplified)
    static struct timeval start_time = {0};
    static bool first_update = true;
    
    if (first_update) {
        gettimeofday(&start_time, NULL);
        first_update = false;
    }
    
    struct timeval current_time;
    gettimeofday(&current_time, NULL);
    
    double elapsed = (current_time.tv_sec - start_time.tv_sec) + 
                   (current_time.tv_usec - start_time.tv_usec) / 1000000.0;
    
    if (elapsed > 0 && bytes_written > 0) {
        double rate = bytes_written / elapsed;  // bytes per second
        uint64_t remaining = total_bytes - bytes_written;
        double eta = remaining / rate;
        
        printf("\r[%s] %.1f%% (%.1f MB/%.1f MB) - %.1f MB/s - ETA: %.0fs", 
               progress->progress_bar,
               progress->percent_complete,
               (double)bytes_written / (1024*1024),
               (double)total_bytes / (1024*1024),
               rate / (1024*1024),
               eta);
    } else {
        printf("\r[%s] %.1f%% (%.1f MB/%.1f MB)", 
               progress->progress_bar,
               progress->percent_complete,
               (double)bytes_written / (1024*1024),
               (double)total_bytes / (1024*1024));
    }
    
    fflush(stdout);
}

// Print ISO information
void print_iso_info(const iso_info_t* iso_info)
{
    if (!iso_info) {
        return;
    }
    
    printf("=== Informations ISO ===\n");
    printf("Fichier: %s\n", iso_info->filepath);
    printf("Taille: %.1f GB (%llu secteurs)\n", 
           (double)iso_info->size_bytes / (1024.0*1024.0*1024.0),
           (unsigned long long)iso_info->size_sectors);
    printf("Volume ID: %s\n", iso_info->volume_id[0] ? iso_info->volume_id : "Inconnu");
    printf("Éditeur: %s\n", iso_info->publisher[0] ? iso_info->publisher : "Inconnu");
    printf("Application: %s\n", iso_info->application[0] ? iso_info->application : "Inconnu");
    printf("Bootable: %s\n", iso_info->is_bootable ? "Oui" : "Non");
}

// Write ISO to device
write_result_t write_iso_to_device(const char* iso_path, const char* device_path, 
                                  write_progress_t* progress)
{
    int iso_fd = -1, device_fd = -1;
    char* write_buffer = NULL;
    write_result_t result = WRITE_SUCCESS;
    uint64_t bytes_written = 0;
    ssize_t bytes_read, bytes_written_this_time;
    struct stat st;
    
    if (!iso_path || !device_path) {
        return WRITE_ERROR_INVALID_ISO;
    }
    
    // Reset cancellation flag
    pthread_mutex_lock(&cancel_mutex);
    write_cancelled = false;
    pthread_mutex_unlock(&cancel_mutex);
    
    printf("=== Écriture ISO ===\n");
    printf("Source: %s\n", iso_path);
    printf("Cible: %s\n", device_path);
    
    // Validate ISO file
    iso_info_t iso_info;
    if (!get_iso_info(iso_path, &iso_info)) {
        return WRITE_ERROR_INVALID_ISO;
    }
    
    print_iso_info(&iso_info);
    
    // Check device space
    if (!check_device_space(device_path, &iso_info)) {
        return WRITE_ERROR_SPACE;
    }
    
    // Validate device
    if (!validate_device_path(device_path)) {
        return WRITE_ERROR_INVALID_DEVICE;
    }
    
    // Open ISO file for reading
    iso_fd = open(iso_path, O_RDONLY);
    if (iso_fd < 0) {
        fprintf(stderr, "Erreur: Impossible d'ouvrir l'ISO %s: %s\n", 
                iso_path, strerror(errno));
        return WRITE_ERROR_OPEN_ISO;
    }
    
    // Open device for writing
    device_fd = open(device_path, O_WRONLY | O_SYNC);
    if (device_fd < 0) {
        fprintf(stderr, "Erreur: Impossible d'ouvrir le périphérique %s: %s\n", 
                device_path, strerror(errno));
        close(iso_fd);
        return WRITE_ERROR_OPEN_DEVICE;
    }
    
    // Allocate write buffer
    write_buffer = malloc(WRITE_BUFFER_SIZE);
    if (!write_buffer) {
        fprintf(stderr, "Erreur: Allocation mémoire impossible\n");
        close(iso_fd);
        close(device_fd);
        return WRITE_ERROR_READ_ISO;
    }
    
    printf("\nDébut de l'écriture...\n");
    
    // Write loop
    while (bytes_written < iso_info.size_bytes) {
        // Check for cancellation
        if (is_write_cancelled()) {
            printf("\nÉcriture annulée par l'utilisateur\n");
            result = WRITE_ERROR_CANCELLED;
            break;
        }
        
        // Read from ISO
        bytes_read = read(iso_fd, write_buffer, WRITE_BUFFER_SIZE);
        if (bytes_read < 0) {
            fprintf(stderr, "\nErreur: Lecture ISO échouée: %s\n", strerror(errno));
            result = WRITE_ERROR_READ_ISO;
            break;
        }
        
        if (bytes_read == 0) {
            // End of file
            break;
        }
        
        // Write to device
        bytes_written_this_time = write(device_fd, write_buffer, bytes_read);
        if (bytes_written_this_time < 0) {
            fprintf(stderr, "\nErreur: Écriture périphérique échouée: %s\n", strerror(errno));
            result = WRITE_ERROR_WRITE_DEVICE;
            break;
        }
        
        if (bytes_written_this_time != bytes_read) {
            fprintf(stderr, "\nErreur: Écriture incomplète: %zd/%zd bytes\n", 
                    bytes_written_this_time, bytes_read);
            result = WRITE_ERROR_WRITE_DEVICE;
            break;
        }
        
        bytes_written += bytes_written_this_time;
        
        // Update progress
        if (progress) {
            update_progress(progress, bytes_written, iso_info.size_bytes);
        }
        
        // Sync periodically (every 10MB)
        if (bytes_written % (10 * 1024 * 1024) == 0) {
            fsync(device_fd);
        }
    }
    
    // Final sync
    if (result == WRITE_SUCCESS) {
        if (fsync(device_fd) != 0) {
            fprintf(stderr, "\nErreur: Synchronisation échouée: %s\n", strerror(errno));
            result = WRITE_ERROR_SYNC;
        } else {
            printf("\nÉcriture terminée avec succès!\n");
        }
    }
    
    // Cleanup
    if (write_buffer) {
        free(write_buffer);
    }
    if (iso_fd >= 0) {
        close(iso_fd);
    }
    if (device_fd >= 0) {
        close(device_fd);
    }
    
    return result;
}
