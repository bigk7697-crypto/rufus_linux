/*
 * Rufus Linux - Verification Module
 * Copyright © 2026 Port Project
 *
 * ISO writing verification and integrity checking
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
#include <openssl/md5.h>
#include <openssl/sha.h>
#include "verification.h"
#include "iso_write.h"
#include "disk_access.h"

// Global cancellation flag
static volatile bool verification_cancelled = false;
static pthread_mutex_t verification_mutex = PTHREAD_MUTEX_INITIALIZER;

// Initialize verification system
bool verification_init(void)
{
    verification_cancelled = false;
    return true;
}

// Cleanup verification system
void verification_cleanup(void)
{
    pthread_mutex_destroy(&verification_mutex);
}

// Cancel ongoing verification
void cancel_verification(void)
{
    pthread_mutex_lock(&verification_mutex);
    verification_cancelled = true;
    pthread_mutex_unlock(&verification_mutex);
}

// Check if verification was cancelled
bool is_verification_cancelled(void)
{
    bool cancelled;
    pthread_mutex_lock(&verification_mutex);
    cancelled = verification_cancelled;
    pthread_mutex_unlock(&verification_mutex);
    return cancelled;
}

// Convert hash bytes to hex string
void hash_to_string(const uint8_t* hash, size_t hash_size, char* string)
{
    for (size_t i = 0; i < hash_size; i++) {
        snprintf(string + (i * 2), 3, "%02x", hash[i]);
    }
    string[hash_size * 2] = '\0';
}

// Compare two hashes
bool compare_hashes(const uint8_t* hash1, const uint8_t* hash2, size_t hash_size)
{
    return memcmp(hash1, hash2, hash_size) == 0;
}

// Calculate hash of ISO file
bool calculate_iso_hash(const char* iso_path, hash_type_t hash_type, uint8_t* hash, char* hash_string)
{
    int fd;
    struct stat st;
    char* buffer;
    ssize_t bytes_read;
    uint64_t total_read = 0;
    
    if (!iso_path || !hash || !hash_string) {
        return false;
    }
    
    // Open ISO file
    fd = open(iso_path, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "Erreur: Impossible d'ouvrir l'ISO %s: %s\n", iso_path, strerror(errno));
        return false;
    }
    
    // Get file size
    if (fstat(fd, &st) != 0) {
        fprintf(stderr, "Erreur: Impossible d'obtenir la taille de l'ISO: %s\n", strerror(errno));
        close(fd);
        return false;
    }
    
    // Allocate buffer
    buffer = malloc(WRITE_BUFFER_SIZE);
    if (!buffer) {
        fprintf(stderr, "Erreur: Allocation mémoire impossible\n");
        close(fd);
        return false;
    }
    
    // Initialize hash context
    MD5_CTX md5_ctx;
    SHA_CTX sha1_ctx;
    SHA256_CTX sha256_ctx;
    
    switch (hash_type) {
        case HASH_MD5:
            MD5_Init(&md5_ctx);
            break;
        case HASH_SHA1:
            SHA1_Init(&sha1_ctx);
            break;
        case HASH_SHA256:
            SHA256_Init(&sha256_ctx);
            break;
        default:
            fprintf(stderr, "Erreur: Type de hash non supporté\n");
            free(buffer);
            close(fd);
            return false;
    }
    
    // Read file and update hash
    while (total_read < st.st_size) {
        // Check for cancellation
        if (is_verification_cancelled()) {
            free(buffer);
            close(fd);
            return false;
        }
        
        bytes_read = read(fd, buffer, WRITE_BUFFER_SIZE);
        if (bytes_read < 0) {
            fprintf(stderr, "Erreur: Lecture ISO échouée: %s\n", strerror(errno));
            free(buffer);
            close(fd);
            return false;
        }
        
        if (bytes_read == 0) {
            break;
        }
        
        // Update hash
        switch (hash_type) {
            case HASH_MD5:
                MD5_Update(&md5_ctx, buffer, bytes_read);
                break;
            case HASH_SHA1:
                SHA1_Update(&sha1_ctx, buffer, bytes_read);
                break;
            case HASH_SHA256:
                SHA256_Update(&sha256_ctx, buffer, bytes_read);
                break;
        }
        
        total_read += bytes_read;
    }
    
    // Finalize hash
    switch (hash_type) {
        case HASH_MD5:
            MD5_Final(hash, &md5_ctx);
            hash_to_string(hash, HASH_MD5_SIZE, hash_string);
            break;
        case HASH_SHA1:
            SHA1_Final(hash, &sha1_ctx);
            hash_to_string(hash, HASH_SHA1_SIZE, hash_string);
            break;
        case HASH_SHA256:
            SHA256_Final(hash, &sha256_ctx);
            hash_to_string(hash, HASH_SHA256_SIZE, hash_string);
            break;
    }
    
    free(buffer);
    close(fd);
    
    return true;
}

// Calculate hash of device (specific range)
bool calculate_device_hash(const char* device_path, uint64_t start_offset, uint64_t size, 
                          hash_type_t hash_type, uint8_t* hash, char* hash_string)
{
    int fd;
    char* buffer;
    ssize_t bytes_read;
    uint64_t total_read = 0;
    off_t offset;
    
    if (!device_path || !hash || !hash_string) {
        return false;
    }
    
    // Open device
    fd = open(device_path, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "Erreur: Impossible d'ouvrir le périphérique %s: %s\n", 
                device_path, strerror(errno));
        return false;
    }
    
    // Seek to start offset
    offset = lseek(fd, start_offset, SEEK_SET);
    if (offset != start_offset) {
        fprintf(stderr, "Erreur: Impossible de se positionner à l'offset %llu: %s\n", 
                (unsigned long long)start_offset, strerror(errno));
        close(fd);
        return false;
    }
    
    // Allocate buffer
    buffer = malloc(WRITE_BUFFER_SIZE);
    if (!buffer) {
        fprintf(stderr, "Erreur: Allocation mémoire impossible\n");
        close(fd);
        return false;
    }
    
    // Initialize hash context
    MD5_CTX md5_ctx;
    SHA_CTX sha1_ctx;
    SHA256_CTX sha256_ctx;
    
    switch (hash_type) {
        case HASH_MD5:
            MD5_Init(&md5_ctx);
            break;
        case HASH_SHA1:
            SHA1_Init(&sha1_ctx);
            break;
        case HASH_SHA256:
            SHA256_Init(&sha256_ctx);
            break;
        default:
            fprintf(stderr, "Erreur: Type de hash non supporté\n");
            free(buffer);
            close(fd);
            return false;
    }
    
    // Read device and update hash
    while (total_read < size) {
        // Check for cancellation
        if (is_verification_cancelled()) {
            free(buffer);
            close(fd);
            return false;
        }
        
        bytes_read = read(fd, buffer, WRITE_BUFFER_SIZE);
        if (bytes_read < 0) {
            fprintf(stderr, "Erreur: Lecture périphérique échouée: %s\n", strerror(errno));
            free(buffer);
            close(fd);
            return false;
        }
        
        if (bytes_read == 0) {
            break;
        }
        
        // Don't read beyond the requested size
        if (total_read + bytes_read > size) {
            bytes_read = size - total_read;
        }
        
        // Update hash
        switch (hash_type) {
            case HASH_MD5:
                MD5_Update(&md5_ctx, buffer, bytes_read);
                break;
            case HASH_SHA1:
                SHA1_Update(&sha1_ctx, buffer, bytes_read);
                break;
            case HASH_SHA256:
                SHA256_Update(&sha256_ctx, buffer, bytes_read);
                break;
        }
        
        total_read += bytes_read;
    }
    
    // Finalize hash
    switch (hash_type) {
        case HASH_MD5:
            MD5_Final(hash, &md5_ctx);
            hash_to_string(hash, HASH_MD5_SIZE, hash_string);
            break;
        case HASH_SHA1:
            SHA1_Final(hash, &sha1_ctx);
            hash_to_string(hash, HASH_SHA1_SIZE, hash_string);
            break;
        case HASH_SHA256:
            SHA256_Final(hash, &sha256_ctx);
            hash_to_string(hash, HASH_SHA256_SIZE, hash_string);
            break;
    }
    
    free(buffer);
    close(fd);
    
    return true;
}

// Update verification progress
void update_verification_progress(verification_progress_t* progress, 
                                  uint64_t bytes_verified, uint64_t total_bytes)
{
    if (!progress) {
        return;
    }
    
    progress->bytes_verified = bytes_verified;
    progress->total_bytes = total_bytes;
    progress->percent_complete = (float)bytes_verified * 100.0f / total_bytes;
    
    // Create progress bar
    int filled = (int)(progress->percent_complete * 80.0f / 100.0f);
    int empty = 80 - filled;
    
    memset(progress->progress_bar, '=', filled);
    memset(progress->progress_bar + filled, '-', empty);
    progress->progress_bar[80] = '\0';
    
    // Calculate verification rate and ETA
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
    
    if (elapsed > 0 && bytes_verified > 0) {
        progress->verification_rate = bytes_verified / elapsed;  // bytes per second
        uint64_t remaining = total_bytes - bytes_verified;
        progress->eta_seconds = remaining / progress->verification_rate;
        
        printf("\r[%s] %.1f%% (%.1f MB/%.1f MB) - %.1f MB/s - ETA: %.0fs", 
               progress->progress_bar,
               progress->percent_complete,
               (double)bytes_verified / (1024*1024),
               (double)total_bytes / (1024*1024),
               progress->verification_rate / (1024*1024),
               progress->eta_seconds);
    } else {
        printf("\r[%s] %.1f%% (%.1f MB/%.1f MB)", 
               progress->progress_bar,
               progress->percent_complete,
               (double)bytes_verified / (1024*1024),
               (double)total_bytes / (1024*1024));
    }
    
    fflush(stdout);
}

// Verify ISO write by comparing hashes
verify_result_t verify_iso_write(const char* iso_path, const char* device_path, 
                                verification_progress_t* progress)
{
    iso_info_t iso_info;
    hash_results_t iso_hashes, device_hashes;
    verify_result_t result = VERIFY_SUCCESS;
    
    if (!iso_path || !device_path) {
        return VERIFY_ERROR_INVALID_ISO;
    }
    
    // Reset cancellation flag
    pthread_mutex_lock(&verification_mutex);
    verification_cancelled = false;
    pthread_mutex_unlock(&verification_mutex);
    
    printf("=== Vérification de l'Écriture ===\n");
    printf("Source: %s\n", iso_path);
    printf("Cible: %s\n", device_path);
    
    // Get ISO information
    if (!get_iso_info(iso_path, &iso_info)) {
        return VERIFY_ERROR_INVALID_ISO;
    }
    
    printf("Taille à vérifier: %.1f GB\n", (double)iso_info.size_bytes / (1024*1024*1024));
    
    // Calculate ISO hashes
    printf("\nCalcul des hashes de l'ISO...\n");
    
    memset(&iso_hashes, 0, sizeof(iso_hashes));
    
    if (calculate_iso_hash(iso_path, HASH_MD5, iso_hashes.md5, iso_hashes.md5_string)) {
        iso_hashes.md5_valid = true;
        printf("  MD5: %s\n", iso_hashes.md5_string);
    }
    
    if (calculate_iso_hash(iso_path, HASH_SHA1, iso_hashes.sha1, iso_hashes.sha1_string)) {
        iso_hashes.sha1_valid = true;
        printf("  SHA1: %s\n", iso_hashes.sha1_string);
    }
    
    if (calculate_iso_hash(iso_path, HASH_SHA256, iso_hashes.sha256, iso_hashes.sha256_string)) {
        iso_hashes.sha256_valid = true;
        printf("  SHA256: %s\n", iso_hashes.sha256_string);
    }
    
    // Calculate device hashes
    printf("\nCalcul des hashes du périphérique...\n");
    
    memset(&device_hashes, 0, sizeof(device_hashes));
    
    if (calculate_device_hash(device_path, 0, iso_info.size_bytes, HASH_MD5, 
                               device_hashes.md5, device_hashes.md5_string)) {
        device_hashes.md5_valid = true;
        printf("  MD5: %s\n", device_hashes.md5_string);
    }
    
    if (calculate_device_hash(device_path, 0, iso_info.size_bytes, HASH_SHA1, 
                               device_hashes.sha1, device_hashes.sha1_string)) {
        device_hashes.sha1_valid = true;
        printf("  SHA1: %s\n", device_hashes.sha1_string);
    }
    
    if (calculate_device_hash(device_path, 0, iso_info.size_bytes, HASH_SHA256, 
                               device_hashes.sha256, device_hashes.sha256_string)) {
        device_hashes.sha256_valid = true;
        printf("  SHA256: %s\n", device_hashes.sha256_string);
    }
    
    // Compare hashes
    printf("\n=== Comparaison des Hashes ===\n");
    
    bool all_match = true;
    
    if (iso_hashes.md5_valid && device_hashes.md5_valid) {
        bool match = compare_hashes(iso_hashes.md5, device_hashes.md5, HASH_MD5_SIZE);
        printf("MD5: %s\n", match ? "CORRESPOND" : "DIFFÉRENT");
        if (!match) all_match = false;
    }
    
    if (iso_hashes.sha1_valid && device_hashes.sha1_valid) {
        bool match = compare_hashes(iso_hashes.sha1, device_hashes.sha1, HASH_SHA1_SIZE);
        printf("SHA1: %s\n", match ? "CORRESPOND" : "DIFFÉRENT");
        if (!match) all_match = false;
    }
    
    if (iso_hashes.sha256_valid && device_hashes.sha256_valid) {
        bool match = compare_hashes(iso_hashes.sha256, device_hashes.sha256, HASH_SHA256_SIZE);
        printf("SHA256: %s\n", match ? "CORRESPOND" : "DIFFÉRENT");
        if (!match) all_match = false;
    }
    
    if (all_match) {
        printf("\n=== VÉRIFICATION RÉUSSIE ===\n");
        printf("L'image ISO a été écrite correctement sur le périphérique.\n");
        result = VERIFY_SUCCESS;
    } else {
        printf("\n=== VÉRIFICATION ÉCHOUÉE ===\n");
        printf("Les hashes ne correspondent pas! L'écriture a échoué.\n");
        result = VERIFY_ERROR_HASH_MISMATCH;
    }
    
    return result;
}

// Print hash results
void print_hash_results(const hash_results_t* results)
{
    if (!results) {
        return;
    }
    
    printf("=== Résultats des Hashes ===\n");
    
    if (results->md5_valid) {
        printf("MD5: %s\n", results->md5_string);
    }
    
    if (results->sha1_valid) {
        printf("SHA1: %s\n", results->sha1_string);
    }
    
    if (results->sha256_valid) {
        printf("SHA256: %s\n", results->sha256_string);
    }
}

// Verify device integrity (quick check)
bool verify_device_integrity(const char* device_path)
{
    int fd;
    uint8_t buffer[512];
    ssize_t bytes_read;
    
    if (!device_path) {
        return false;
    }
    
    fd = open(device_path, O_RDONLY);
    if (fd < 0) {
        return false;
    }
    
    // Read first sector (MBR)
    bytes_read = read(fd, buffer, 512);
    close(fd);
    
    if (bytes_read != 512) {
        return false;
    }
    
    // Basic MBR check (signature 0x55AA at offset 510)
    return (buffer[510] == 0x55 && buffer[511] == 0xAA);
}
