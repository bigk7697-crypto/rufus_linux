/*
 * Rufus Linux - Verification Module
 * Copyright © 2026 Port Project
 *
 * ISO writing verification and integrity checking
 */

#ifndef VERIFICATION_H
#define VERIFICATION_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define HASH_MD5_SIZE 16
#define HASH_SHA1_SIZE 20
#define HASH_SHA256_SIZE 32
#define MAX_HASH_STRING 65

typedef enum {
    HASH_MD5 = 0,
    HASH_SHA1,
    HASH_SHA256,
    HASH_MAX
} hash_type_t;

typedef struct {
    uint8_t md5[HASH_MD5_SIZE];
    uint8_t sha1[HASH_SHA1_SIZE];
    uint8_t sha256[HASH_SHA256_SIZE];
    char md5_string[MAX_HASH_STRING];
    char sha1_string[MAX_HASH_STRING];
    char sha256_string[MAX_HASH_STRING];
    bool md5_valid;
    bool sha1_valid;
    bool sha256_valid;
} hash_results_t;

typedef struct {
    uint64_t bytes_verified;
    uint64_t total_bytes;
    float percent_complete;
    char progress_bar[81];
    double verification_rate;
    double eta_seconds;
    int errors_found;
} verification_progress_t;

typedef enum {
    VERIFY_SUCCESS = 0,
    VERIFY_ERROR_INVALID_ISO = -1,
    VERIFY_ERROR_INVALID_DEVICE = -2,
    VERIFY_ERROR_OPEN_ISO = -3,
    VERIFY_ERROR_OPEN_DEVICE = -4,
    VERIFY_ERROR_READ_ISO = -5,
    VERIFY_ERROR_READ_DEVICE = -6,
    VERIFY_ERROR_HASH_MISMATCH = -7,
    VERIFY_ERROR_CANCELLED = -8,
    VERIFY_ERROR_SIZE_MISMATCH = -9
} verify_result_t;

// Initialize verification system
bool verification_init(void);

// Cleanup verification system
void verification_cleanup(void);

// Calculate hash of ISO file
bool calculate_iso_hash(const char* iso_path, hash_type_t hash_type, uint8_t* hash, char* hash_string);

// Calculate hash of device (specific range)
bool calculate_device_hash(const char* device_path, uint64_t start_offset, uint64_t size, 
                          hash_type_t hash_type, uint8_t* hash, char* hash_string);

// Verify ISO write by comparing hashes
verify_result_t verify_iso_write(const char* iso_path, const char* device_path, 
                                verification_progress_t* progress);

// Update verification progress
void update_verification_progress(verification_progress_t* progress, 
                                  uint64_t bytes_verified, uint64_t total_bytes);

// Convert hash bytes to hex string
void hash_to_string(const uint8_t* hash, size_t hash_size, char* string);

// Compare two hashes
bool compare_hashes(const uint8_t* hash1, const uint8_t* hash2, size_t hash_size);

// Print hash results
void print_hash_results(const hash_results_t* results);

// Verify device integrity (quick check)
bool verify_device_integrity(const char* device_path);

// Cancel ongoing verification
void cancel_verification(void);

// Check if verification was cancelled
bool is_verification_cancelled(void);

#endif // VERIFICATION_H
