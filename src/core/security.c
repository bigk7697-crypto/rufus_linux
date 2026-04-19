/*
 * Rufus Linux - Advanced Security Module
 * Copyright © 2026 Port Project
 *
 * Advanced security features, logging, and audit
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <time.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/mount.h>
#include <sys/utsname.h>
#include <openssl/sha.h>
#include "security.h"
#include "disk_access.h"

// Global state
static log_buffer_t log_buffer = {0};
static audit_buffer_t audit_buffer = {0};
static log_level_t current_log_level = LOG_LEVEL_INFO;
static bool security_monitoring_enabled = false;
static bool emergency_stop_active = false;
static pthread_mutex_t security_mutex = PTHREAD_MUTEX_INITIALIZER;

// Log level strings
static const char* log_level_strings[LOG_LEVEL_MAX] = {
    "DEBUG", "INFO", "WARNING", "ERROR", "CRITICAL"
};

// Audit action strings
static const char* audit_action_strings[AUDIT_ACTION_MAX] = {
    "USB_DETECT", "DEVICE_CHECK", "BACKUP_MBR", "WRITE_START",
    "WRITE_SUCCESS", "WRITE_FAILED", "VERIFY_START", "VERIFY_SUCCESS", "VERIFY_FAILED"
};

// Initialize security system
bool security_init(void)
{
    pthread_mutex_lock(&security_mutex);
    
    memset(&log_buffer, 0, sizeof(log_buffer));
    memset(&audit_buffer, 0, sizeof(audit_buffer));
    
    log_buffer.initialized = true;
    audit_buffer.initialized = true;
    
    // Set default audit file
    snprintf(audit_buffer.audit_file, sizeof(audit_buffer.audit_file), 
             "%s/.rufus_audit.log", getenv("HOME") ? getenv("HOME") : "/tmp");
    
    pthread_mutex_unlock(&security_mutex);
    
    log_info("Security system initialized");
    return true;
}

// Cleanup security system
void security_cleanup(void)
{
    pthread_mutex_lock(&security_mutex);
    
    // Flush logs and audit before cleanup
    flush_logs();
    flush_audit();
    
    log_buffer.initialized = false;
    audit_buffer.initialized = false;
    
    pthread_mutex_unlock(&security_mutex);
    
    pthread_mutex_destroy(&security_mutex);
}

// Get current user information
bool get_current_user_info(char* username, size_t username_size, char* hostname, size_t hostname_size)
{
    struct passwd* pw;
    struct utsname uts;
    
    if (!username || !hostname) {
        return false;
    }
    
    // Get username
    pw = getpwuid(getuid());
    if (pw && pw->pw_name) {
        strncpy(username, pw->pw_name, username_size - 1);
        username[username_size - 1] = '\0';
    } else {
        strncpy(username, "unknown", username_size - 1);
        username[username_size - 1] = '\0';
    }
    
    // Get hostname
    if (uname(&uts) == 0) {
        strncpy(hostname, uts.nodename, hostname_size - 1);
        hostname[hostname_size - 1] = '\0';
    } else {
        strncpy(hostname, "unknown", hostname_size - 1);
        hostname[hostname_size - 1] = '\0';
    }
    
    return true;
}

// Add log entry
static void add_log_entry(log_level_t level, const char* message)
{
    if (!log_buffer.initialized || level < current_log_level) {
        return;
    }
    
    pthread_mutex_lock(&security_mutex);
    
    log_entry_t* entry = &log_buffer.entries[log_buffer.head];
    
    entry->timestamp = time(NULL);
    entry->level = level;
    strncpy(entry->message, message, MAX_LOG_MESSAGE - 1);
    entry->message[MAX_LOG_MESSAGE - 1] = '\0';
    
    get_current_user_info(entry->username, sizeof(entry->username),
                         entry->hostname, sizeof(entry->hostname));
    entry->pid = getpid();
    
    log_buffer.head = (log_buffer.head + 1) % MAX_LOG_ENTRIES;
    if (log_buffer.count < MAX_LOG_ENTRIES) {
        log_buffer.count++;
    }
    
    pthread_mutex_unlock(&security_mutex);
    
    // Also output to stderr for critical messages
    if (level >= LOG_LEVEL_ERROR) {
        fprintf(stderr, "[%s] %s\n", log_level_strings[level], message);
    }
}

// Log message with formatting
void log_message(log_level_t level, const char* format, ...)
{
    char message[MAX_LOG_MESSAGE];
    va_list args;
    
    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);
    
    add_log_entry(level, message);
}

void log_debug(const char* format, ...)
{
    char message[MAX_LOG_MESSAGE];
    va_list args;
    
    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);
    
    add_log_entry(LOG_LEVEL_DEBUG, message);
}

void log_info(const char* format, ...)
{
    char message[MAX_LOG_MESSAGE];
    va_list args;
    
    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);
    
    add_log_entry(LOG_LEVEL_INFO, message);
}

void log_warning(const char* format, ...)
{
    char message[MAX_LOG_MESSAGE];
    va_list args;
    
    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);
    
    add_log_entry(LOG_LEVEL_WARNING, message);
}

void log_error(const char* format, ...)
{
    char message[MAX_LOG_MESSAGE];
    va_list args;
    
    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);
    
    add_log_entry(LOG_LEVEL_ERROR, message);
}

void log_critical(const char* format, ...)
{
    char message[MAX_LOG_MESSAGE];
    va_list args;
    
    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);
    
    add_log_entry(LOG_LEVEL_CRITICAL, message);
}

// Add audit entry
static bool add_audit_entry(audit_action_t action, const char* device_path, const char* iso_path,
                           uint64_t bytes_processed, bool success, const char* details)
{
    if (!audit_buffer.initialized) {
        return false;
    }
    
    pthread_mutex_lock(&security_mutex);
    
    audit_entry_t* entry = &audit_buffer.entries[audit_buffer.head];
    
    entry->timestamp = time(NULL);
    entry->action = action;
    entry->bytes_processed = bytes_processed;
    entry->success = success;
    
    if (device_path) {
        strncpy(entry->device_path, device_path, MAX_AUDIT_FILEPATH - 1);
        entry->device_path[MAX_AUDIT_FILEPATH - 1] = '\0';
    } else {
        entry->device_path[0] = '\0';
    }
    
    if (iso_path) {
        strncpy(entry->iso_path, iso_path, MAX_AUDIT_FILEPATH - 1);
        entry->iso_path[MAX_AUDIT_FILEPATH - 1] = '\0';
    } else {
        entry->iso_path[0] = '\0';
    }
    
    if (details) {
        strncpy(entry->details, details, MAX_LOG_MESSAGE - 1);
        entry->details[MAX_LOG_MESSAGE - 1] = '\0';
    } else {
        entry->details[0] = '\0';
    }
    
    get_current_user_info(entry->username, sizeof(entry->username),
                         entry->hostname, sizeof(entry->hostname));
    entry->pid = getpid();
    
    audit_buffer.head = (audit_buffer.head + 1) % MAX_LOG_ENTRIES;
    if (audit_buffer.count < MAX_LOG_ENTRIES) {
        audit_buffer.count++;
    }
    
    pthread_mutex_unlock(&security_mutex);
    
    return true;
}

// Audit action
bool audit_action(audit_action_t action, const char* device_path, const char* iso_path,
                  uint64_t bytes_processed, bool success, const char* details)
{
    char audit_message[MAX_LOG_MESSAGE];
    
    snprintf(audit_message, sizeof(audit_message), 
             "AUDIT: %s device=%s iso=%s bytes=%llu success=%s %s",
             audit_action_strings[action],
             device_path ? device_path : "none",
             iso_path ? iso_path : "none",
             (unsigned long long)bytes_processed,
             success ? "true" : "false",
             details ? details : "");
    
    log_info("%s", audit_message);
    
    return add_audit_entry(action, device_path, iso_path, bytes_processed, success, details);
}

// Enhanced device safety checks
bool enhanced_device_safety_check(const char* device_path)
{
    if (!device_path) {
        return false;
    }
    
    log_info("Enhanced safety check for device: %s", device_path);
    
    // Check system critical paths
    if (!check_system_critical_paths(device_path)) {
        log_error("Device failed system critical path check: %s", device_path);
        return false;
    }
    
    // Check running processes
    if (!check_running_processes(device_path)) {
        log_error("Device failed running processes check: %s", device_path);
        return false;
    }
    
    // Check network mounts
    if (!check_network_mounts(device_path)) {
        log_error("Device failed network mounts check: %s", device_path);
        return false;
    }
    
    log_info("Device passed enhanced safety checks: %s", device_path);
    return true;
}

// Check system critical paths
bool check_system_critical_paths(const char* device_path)
{
    disk_info_t disk_info;
    
    if (!get_disk_info(device_path, &disk_info)) {
        return false;
    }
    
    // Check for system mount points
    const char* critical_mounts[] = {
        "/", "/boot", "/home", "/usr", "/var", "/opt", "/srv", NULL
    };
    
    for (int i = 0; critical_mounts[i]; i++) {
        for (int j = 0; j < disk_info.mount_point_count; j++) {
            if (strcmp(disk_info.mount_points[j], critical_mounts[i]) == 0) {
                log_error("CRITICAL: Device contains system mount point %s", critical_mounts[i]);
                return false;
            }
        }
    }
    
    // Check device name patterns
    const char* device_name = strrchr(device_path, '/');
    if (device_name) {
        device_name++;
        
        // Never allow /dev/sda (primary system disk)
        if (strcmp(device_name, "sda") == 0) {
            log_error("CRITICAL: Device is primary system disk: %s", device_path);
            return false;
        }
        
        // Be cautious with nvme0n1
        if (strncmp(device_name, "nvme0n1", 7) == 0) {
            log_error("CRITICAL: Device is primary NVMe system disk: %s", device_path);
            return false;
        }
        
        // Be cautious with vda (primary virtual disk)
        if (strcmp(device_name, "vda") == 0) {
            log_error("CRITICAL: Device is primary virtual disk: %s", device_path);
            return false;
        }
    }
    
    return true;
}

// Check running processes that might be using the device
bool check_running_processes(const char* device_path)
{
    FILE* fp;
    char line[1024];
    char device_name[256];
    int processes_found = 0;
    
    if (!device_path) {
        return false;
    }
    
    // Extract device name
    const char* name_start = strrchr(device_path, '/');
    if (!name_start) {
        return false;
    }
    name_start++;
    strncpy(device_name, name_start, sizeof(device_name) - 1);
    device_name[sizeof(device_name) - 1] = '\0';
    
    // Check /proc/mounts for active mounts
    fp = fopen("/proc/mounts", "r");
    if (fp) {
        while (fgets(line, sizeof(line), fp)) {
            if (strstr(line, device_name)) {
                processes_found++;
                log_warning("Device %s is actively mounted", device_path);
            }
        }
        fclose(fp);
    }
    
    // Check lsof output (if available)
    fp = popen("lsof 2>/dev/null", "r");
    if (fp) {
        while (fgets(line, sizeof(line), fp)) {
            if (strstr(line, device_name)) {
                processes_found++;
                log_warning("Device %s is being used by a process", device_path);
            }
        }
        pclose(fp);
    }
    
    if (processes_found > 0) {
        log_error("Device %s has %d active processes/mounts", device_path, processes_found);
        return false;
    }
    
    return true;
}

// Check network mounts
bool check_network_mounts(const char* device_path)
{
    FILE* fp;
    char line[1024];
    char device_name[256];
    
    if (!device_path) {
        return false;
    }
    
    // Extract device name
    const char* name_start = strrchr(device_path, '/');
    if (!name_start) {
        return false;
    }
    name_start++;
    strncpy(device_name, name_start, sizeof(device_name) - 1);
    device_name[sizeof(device_name) - 1] = '\0';
    
    // Check /proc/mounts for network mounts
    fp = fopen("/proc/mounts", "r");
    if (!fp) {
        return true; // Assume safe if we can't check
    }
    
    while (fgets(line, sizeof(line), fp)) {
        char mount_source[256], mount_target[256], mount_type[64];
        
        if (sscanf(line, "%255s %255s %63s", mount_source, mount_target, mount_type) == 3) {
            // Check if this mount point involves our device
            if (strstr(mount_source, device_name)) {
                // Check if it's a network filesystem
                if (strcmp(mount_type, "nfs") == 0 || strcmp(mount_type, "nfs4") == 0 ||
                    strcmp(mount_type, "cifs") == 0 || strcmp(mount_type, "smbfs") == 0 ||
                    strcmp(mount_type, "sshfs") == 0) {
                    log_error("CRITICAL: Device %s is part of network mount %s (%s)", 
                             device_path, mount_target, mount_type);
                    fclose(fp);
                    return false;
                }
            }
        }
    }
    
    fclose(fp);
    return true;
}

// Create operation signature
bool create_operation_signature(const char* filepath, uint8_t* signature)
{
    FILE* fp;
    char buffer[1024];
    size_t bytes_read;
    SHA256_CTX sha256;
    
    if (!filepath || !signature) {
        return false;
    }
    
    fp = fopen(filepath, "rb");
    if (!fp) {
        return false;
    }
    
    SHA256_Init(&sha256);
    
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
        SHA256_Update(&sha256, buffer, bytes_read);
    }
    
    SHA256_Final(signature, &sha256);
    fclose(fp);
    
    return true;
}

// Verify operation signature
bool verify_operation_signature(const char* filepath, const uint8_t* signature)
{
    uint8_t current_signature[SHA256_DIGEST_LENGTH];
    
    if (!filepath || !signature) {
        return false;
    }
    
    if (!create_operation_signature(filepath, current_signature)) {
        return false;
    }
    
    return memcmp(signature, current_signature, SHA256_DIGEST_LENGTH) == 0;
}

// Security policy enforcement
bool enforce_security_policy(const char* device_path)
{
    // Check if user has sufficient permissions
    if (!check_user_permissions(device_path)) {
        log_error("User lacks sufficient permissions for device: %s", device_path);
        return false;
    }
    
    // Check if security monitoring is enabled
    if (security_monitoring_enabled) {
        log_info("Security monitoring is enabled - enforcing strict policies");
        return enhanced_device_safety_check(device_path);
    }
    
    return true;
}

// Check user permissions
bool check_user_permissions(const char* device_path)
{
    // Check if running as root (required for disk operations)
    if (getuid() != 0) {
        log_warning("Not running as root - some operations may fail");
        // Don't fail here, let the actual operation fail if needed
    }
    
    // Check if we can read the device
    if (access(device_path, R_OK) != 0) {
        log_error("No read access to device: %s", device_path);
        return false;
    }
    
    // Check if we can write to the device
    if (access(device_path, W_OK) != 0) {
        log_error("No write access to device: %s", device_path);
        return false;
    }
    
    return true;
}

// Log management functions
void flush_logs(void)
{
    pthread_mutex_lock(&security_mutex);
    
    if (!log_buffer.initialized) {
        pthread_mutex_unlock(&security_mutex);
        return;
    }
    
    // Save logs to file
    char log_file[512];
    snprintf(log_file, sizeof(log_file), "%s/.rufus.log", getenv("HOME") ? getenv("HOME") : "/tmp");
    
    FILE* fp = fopen(log_file, "a");
    if (fp) {
        for (int i = 0; i < log_buffer.count; i++) {
            log_entry_t* entry = &log_buffer.entries[(log_buffer.tail + i) % MAX_LOG_ENTRIES];
            fprintf(fp, "[%ld] [%s] [%s@%s:%d] %s\n",
                   entry->timestamp, log_level_strings[entry->level],
                   entry->username, entry->hostname, entry->pid,
                   entry->message);
        }
        fclose(fp);
    }
    
    pthread_mutex_unlock(&security_mutex);
}

void clear_logs(void)
{
    pthread_mutex_lock(&security_mutex);
    
    log_buffer.count = 0;
    log_buffer.head = 0;
    log_buffer.tail = 0;
    
    pthread_mutex_unlock(&security_mutex);
    
    log_info("Logs cleared");
}

void print_recent_logs(int count)
{
    pthread_mutex_lock(&security_mutex);
    
    if (!log_buffer.initialized || log_buffer.count == 0) {
        printf("No logs available\n");
        pthread_mutex_unlock(&security_mutex);
        return;
    }
    
    int start = (log_buffer.count > count) ? log_buffer.count - count : 0;
    
    printf("=== Recent Logs (last %d entries) ===\n", count);
    
    for (int i = start; i < log_buffer.count; i++) {
        log_entry_t* entry = &log_buffer.entries[(log_buffer.tail + i) % MAX_LOG_ENTRIES];
        
        char time_str[64];
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", localtime(&entry->timestamp));
        
        printf("[%s] [%s] [%s@%s:%d] %s\n",
               time_str, log_level_strings[entry->level],
               entry->username, entry->hostname, entry->pid,
               entry->message);
    }
    
    pthread_mutex_unlock(&security_mutex);
}

bool save_logs_to_file(const char* filename)
{
    pthread_mutex_lock(&security_mutex);
    
    if (!log_buffer.initialized || !filename) {
        pthread_mutex_unlock(&security_mutex);
        return false;
    }
    
    FILE* fp = fopen(filename, "w");
    if (!fp) {
        pthread_mutex_unlock(&security_mutex);
        return false;
    }
    
    for (int i = 0; i < log_buffer.count; i++) {
        log_entry_t* entry = &log_buffer.entries[(log_buffer.tail + i) % MAX_LOG_ENTRIES];
        fprintf(fp, "[%ld] [%s] [%s@%s:%d] %s\n",
               entry->timestamp, log_level_strings[entry->level],
               entry->username, entry->hostname, entry->pid,
               entry->message);
    }
    
    fclose(fp);
    pthread_mutex_unlock(&security_mutex);
    
    log_info("Logs saved to file: %s", filename);
    return true;
}

// Audit management functions
void flush_audit(void)
{
    pthread_mutex_lock(&security_mutex);
    
    if (!audit_buffer.initialized) {
        pthread_mutex_unlock(&security_mutex);
        return;
    }
    
    FILE* fp = fopen(audit_buffer.audit_file, "a");
    if (fp) {
        for (int i = 0; i < audit_buffer.count; i++) {
            audit_entry_t* entry = &audit_buffer.entries[(audit_buffer.tail + i) % MAX_LOG_ENTRIES];
            
            char time_str[64];
            strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", localtime(&entry->timestamp));
            
            fprintf(fp, "[%s] [%s] [%s@%s:%d] action=%s device=%s iso=%s bytes=%llu success=%s %s\n",
                   time_str, audit_action_strings[entry->action],
                   entry->username, entry->hostname, entry->pid,
                   entry->device_path, entry->iso_path,
                   (unsigned long long)entry->bytes_processed,
                   entry->success ? "true" : "false",
                   entry->details);
        }
        fclose(fp);
    }
    
    pthread_mutex_unlock(&security_mutex);
}

void clear_audit(void)
{
    pthread_mutex_lock(&security_mutex);
    
    audit_buffer.count = 0;
    audit_buffer.head = 0;
    audit_buffer.tail = 0;
    
    pthread_mutex_unlock(&security_mutex);
    
    log_info("Audit cleared");
}

void print_recent_audit(int count)
{
    pthread_mutex_lock(&security_mutex);
    
    if (!audit_buffer.initialized || audit_buffer.count == 0) {
        printf("No audit entries available\n");
        pthread_mutex_unlock(&security_mutex);
        return;
    }
    
    int start = (audit_buffer.count > count) ? audit_buffer.count - count : 0;
    
    printf("=== Recent Audit Entries (last %d entries) ===\n", count);
    
    for (int i = start; i < audit_buffer.count; i++) {
        audit_entry_t* entry = &audit_buffer.entries[(audit_buffer.tail + i) % MAX_LOG_ENTRIES];
        
        char time_str[64];
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", localtime(&entry->timestamp));
        
        printf("[%s] [%s] [%s@%s:%d] action=%s device=%s iso=%s bytes=%llu success=%s %s\n",
               time_str, audit_action_strings[entry->action],
               entry->username, entry->hostname, entry->pid,
               entry->device_path, entry->iso_path,
               (unsigned long long)entry->bytes_processed,
               entry->success ? "true" : "false",
               entry->details);
    }
    
    pthread_mutex_unlock(&security_mutex);
}

bool save_audit_to_file(const char* filename)
{
    pthread_mutex_lock(&security_mutex);
    
    if (!audit_buffer.initialized || !filename) {
        pthread_mutex_unlock(&security_mutex);
        return false;
    }
    
    FILE* fp = fopen(filename, "w");
    if (!fp) {
        pthread_mutex_unlock(&security_mutex);
        return false;
    }
    
    for (int i = 0; i < audit_buffer.count; i++) {
        audit_entry_t* entry = &audit_buffer.entries[(audit_buffer.tail + i) % MAX_LOG_ENTRIES];
        
        char time_str[64];
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", localtime(&entry->timestamp));
        
        fprintf(fp, "[%s] [%s] [%s@%s:%d] action=%s device=%s iso=%s bytes=%llu success=%s %s\n",
               time_str, audit_action_strings[entry->action],
               entry->username, entry->hostname, entry->pid,
               entry->device_path, entry->iso_path,
               (unsigned long long)entry->bytes_processed,
               entry->success ? "true" : "false",
               entry->details);
    }
    
    fclose(fp);
    pthread_mutex_unlock(&security_mutex);
    
    log_info("Audit saved to file: %s", filename);
    return true;
}

// Security monitoring
bool enable_security_monitoring(void)
{
    security_monitoring_enabled = true;
    log_info("Security monitoring enabled");
    return true;
}

bool disable_security_monitoring(void)
{
    security_monitoring_enabled = false;
    log_info("Security monitoring disabled");
    return true;
}

bool is_security_monitoring_enabled(void)
{
    return security_monitoring_enabled;
}

// Emergency functions
void emergency_stop_all_operations(void)
{
    emergency_stop_active = true;
    log_critical("EMERGENCY STOP ACTIVATED - All operations halted");
    
    // Cancel any ongoing operations
    cancel_write_operation();
    cancel_verification();
}

bool is_emergency_stop_active(void)
{
    return emergency_stop_active;
}

void reset_emergency_stop(void)
{
    emergency_stop_active = false;
    log_info("Emergency stop reset");
}

// Configuration functions
bool set_log_level(log_level_t level)
{
    if (level >= LOG_LEVEL_DEBUG && level < LOG_LEVEL_MAX) {
        current_log_level = level;
        log_info("Log level set to: %s", log_level_strings[level]);
        return true;
    }
    return false;
}

log_level_t get_log_level(void)
{
    return current_log_level;
}

bool set_audit_file(const char* filepath)
{
    if (!filepath) {
        return false;
    }
    
    pthread_mutex_lock(&security_mutex);
    strncpy(audit_buffer.audit_file, filepath, MAX_AUDIT_FILEPATH - 1);
    audit_buffer.audit_file[MAX_AUDIT_FILEPATH - 1] = '\0';
    pthread_mutex_unlock(&security_mutex);
    
    log_info("Audit file set to: %s", filepath);
    return true;
}

const char* get_audit_file(void)
{
    return audit_buffer.audit_file;
}
