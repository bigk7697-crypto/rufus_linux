/*
 * Rufus Linux - User Interface Module
 * Copyright © 2026 Port Project
 *
 * Enhanced user interface with interactive menus and progress displays
 */

#ifndef UI_H
#define UI_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define MAX_MENU_ITEMS 20
#define MAX_MENU_TEXT 256
#define MAX_INPUT_BUFFER 512
#define MAX_STATUS_TEXT 256
#define PROGRESS_BAR_WIDTH 50

typedef enum {
    UI_MODE_CLI = 0,
    UI_MODE_INTERACTIVE,
    UI_MODE_SILENT,
    UI_MODE_MAX
} ui_mode_t;

typedef enum {
    MENU_TYPE_MAIN = 0,
    MENU_TYPE_DEVICE,
    MENU_TYPE_FILESYSTEM,
    MENU_TYPE_ISO,
    MENU_TYPE_SECURITY,
    MENU_TYPE_SETTINGS,
    MENU_TYPE_MAX
} menu_type_t;

typedef struct {
    int id;
    char title[MAX_MENU_TEXT];
    char description[MAX_MENU_TEXT];
    bool enabled;
    bool requires_device;
    bool requires_iso;
    char shortcut;
} menu_item_t;

typedef struct {
    menu_type_t type;
    char title[MAX_MENU_TEXT];
    menu_item_t items[MAX_MENU_ITEMS];
    int item_count;
    int selected_item;
    bool show_descriptions;
} menu_t;

typedef struct {
    uint64_t current;
    uint64_t total;
    float percent;
    char status[MAX_STATUS_TEXT];
    char time_remaining[64];
    char speed[64];
    bool active;
    bool cancelled;
} progress_info_t;

typedef struct {
    ui_mode_t mode;
    bool color_enabled;
    bool unicode_enabled;
    bool animations_enabled;
    int verbosity_level;
    char status_text[MAX_STATUS_TEXT];
    progress_info_t progress;
} ui_context_t;

// Initialize UI system
bool ui_init(ui_mode_t mode);

// Cleanup UI system
void ui_cleanup(void);

// Set UI mode
bool ui_set_mode(ui_mode_t mode);

// Get current UI mode
ui_mode_t ui_get_mode(void);

// Clear screen
void ui_clear_screen(void);

// Print header
void ui_print_header(const char* title);

// Print footer
void ui_print_footer(void);

// Show main menu
int ui_show_main_menu(void);

// Show device selection menu
int ui_show_device_menu(const char* title, bool allow_refresh);

// Show filesystem selection menu
int ui_show_filesystem_menu(void);

// Show ISO selection
bool ui_select_iso_file(char* filepath, size_t filepath_size);

// Show confirmation dialog
bool ui_confirm_dialog(const char* title, const char* message, const char* details);

// Show input dialog
bool ui_input_dialog(const char* title, const char* prompt, char* buffer, size_t buffer_size);

// Show progress bar
void ui_show_progress(const progress_info_t* progress);

// Update progress
void ui_update_progress(uint64_t current, uint64_t total, const char* status);

// Hide progress
void ui_hide_progress(void);

// Show status message
void ui_show_status(const char* message, bool temporary);

// Show error message
void ui_show_error(const char* message, const char* details);

// Show warning message
void ui_show_warning(const char* message, const char* details);

// Show success message
void ui_show_success(const char* message, const char* details);

// Show info message
void ui_show_info(const char* message, const char* details);

// Show device list
void ui_show_device_list(const char* title);

// Show ISO information
void ui_show_iso_info(const char* filepath);

// Show filesystem information
void ui_show_filesystem_info(const char* fs_type);

// Show security status
void ui_show_security_status(void);

// Show settings menu
int ui_show_settings_menu(void);

// Interactive mode main loop
int ui_interactive_main_loop(void);

// Handle keyboard input
char ui_get_keypress(void);

// Parse menu selection
int ui_parse_menu_selection(const char* input);

// Validate user input
bool ui_validate_input(const char* input, int min_length, int max_length);

// Format bytes for display
void ui_format_bytes(uint64_t bytes, char* buffer, size_t buffer_size);

// Format time for display
void ui_format_time(uint64_t seconds, char* buffer, size_t buffer_size);

// Format speed for display
void ui_format_speed(double bytes_per_second, char* buffer, size_t buffer_size);

// Create progress bar string
void ui_create_progress_bar(float percent, char* buffer, size_t buffer_size, bool animated);

// Show spinning cursor
void ui_show_spinner(bool active);

// Color support functions
bool ui_supports_color(void);
void ui_set_color(int color);
void ui_reset_color(void);

// Unicode support functions
bool ui_supports_unicode(void);
void ui_print_unicode(const char* text);

// Animation functions
void ui_start_animation(const char* type);
void ui_stop_animation(void);
void ui_update_animation(void);

// Help system
void ui_show_help(void);
void ui_show_version(void);
void ui_show_about(void);

// Configuration
bool ui_load_config(void);
bool ui_save_config(void);
void ui_reset_config(void);

// Logging integration
void ui_log_event(const char* event, const char* details);

#endif // UI_H
