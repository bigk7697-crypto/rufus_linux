/*
 * Rufus Linux - Advanced Security Module
 * Copyright © 2026 Port Project
 *
 * Advanced security features, logging, and audit
 */

#ifndef SECURITY_H
#define SECURITY_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#define MAX_LOG_ENTRIES 1000
#define MAX_LOG_MESSAGE 512
#define MAX_AUDIT_FILEPATH 512
#define MAX_USERNAME 64
#define MAX_HOSTNAME 256

typedef enum {
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARNING,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_CRITICAL,
    LOG_LEVEL_MAX
} log_level_t;

typedef enum {
    AUDIT_ACTION_USB_DETECT = 0,
    AUDIT_ACTION_DEVICE_CHECK,
    AUDIT_ACTION_BACKUP_MBR,
    AUDIT_ACTION_WRITE_START,
    AUDIT_ACTION_WRITE_SUCCESS,
    AUDIT_ACTION_WRITE_FAILED,
    AUDIT_ACTION_VERIFY_START,
    AUDIT_ACTION_VERIFY_SUCCESS,
    AUDIT_ACTION_VERIFY_FAILED,
    AUDIT_ACTION_MAX
} audit_action_t;

typedef struct {
    time_t timestamp;
    log_level_t level;
    char message[MAX_LOG_MESSAGE];
    char username[MAX_USERNAME];
    char hostname[MAX_HOSTNAME];
    pid_t pid;
} log_entry_t;

typedef struct {
    time_t timestamp;
    audit_action_t action;
    char device_path[MAX_AUDIT_FILEPATH];
    char iso_path[MAX_AUDIT_FILEPATH];
    char username[MAX_USERNAME];
    char hostname[MAX_HOSTNAME];
    pid_t pid;
    uint64_t bytes_processed;
    bool success;
    char details[MAX_LOG_MESSAGE];
} audit_entry_t;

typedef struct {
    log_entry_t entries[MAX_LOG_ENTRIES];
    int count;
    int head;
    int tail;
    bool initialized;
} log_buffer_t;

typedef struct {
    audit_entry_t entries[MAX_LOG_ENTRIES];
    int count;
    int head;
    int tail;
    bool initialized;
    char audit_file[MAX_AUDIT_FILEPATH];
} audit_buffer_t;

// Initialize security system
bool security_init(void);

// Cleanup security system
void security_cleanup(void);

// Logging functions
void log_message(log_level_t level, const char* format, ...);
void log_debug(const char* format, ...);
void log_info(const char* format, ...);
void log_warning(const char* format, ...);
void log_error(const char* format, ...);
void log_critical(const char* format, ...);

// Audit functions
bool audit_action(audit_action_t action, const char* device_path, const char* iso_path, 
                   uint64_t bytes_processed, bool success, const char* details);

// Get current user information
bool get_current_user_info(char* username, size_t username_size, char* hostname, size_t hostname_size);

// Enhanced device safety checks
bool enhanced_device_safety_check(const char* device_path);
bool check_system_critical_paths(const char* device_path);
bool check_running_processes(const char* device_path);
bool check_network_mounts(const char* device_path);

// File integrity monitoring
bool create_operation_signature(const char* filepath, uint8_t* signature);
bool verify_operation_signature(const char* filepath, const uint8_t* signature);

// Security policy enforcement
bool enforce_security_policy(const char* device_path);
bool check_user_permissions(const char* device_path);

// Log management
void flush_logs(void);
void clear_logs(void);
void print_recent_logs(int count);
bool save_logs_to_file(const char* filename);
bool load_logs_from_file(const char* filename);

// Audit management
void flush_audit(void);
void clear_audit(void);
void print_recent_audit(int count);
bool save_audit_to_file(const char* filename);

// Security monitoring
bool enable_security_monitoring(void);
bool disable_security_monitoring(void);
bool is_security_monitoring_enabled(void);

// Emergency functions
void emergency_stop_all_operations(void);
bool is_emergency_stop_active(void);
void reset_emergency_stop(void);

// Configuration
bool set_log_level(log_level_t level);
log_level_t get_log_level(void);
bool set_audit_file(const char* filepath);
const char* get_audit_file(void);

#endif // SECURITY_H
