/*
 * Rufus Linux - User Interface Module
 * Copyright © 2026 Port Project
 *
 * Enhanced user interface with interactive menus and progress displays
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <time.h>
#include <signal.h>
#include <pthread.h>
#include "ui.h"
#include "disk_access.h"
#include "filesystem.h"
#include "security.h"
#include "iso_write.h"
#include "verification.h"

// Global UI context
static ui_context_t ui_context = {0};
static pthread_mutex_t ui_mutex = PTHREAD_MUTEX_INITIALIZER;
static bool ui_animation_running = false;
static pthread_t animation_thread;

// Terminal capabilities
static bool supports_color = false;
static bool supports_unicode = false;
static int terminal_width = 80;
static int terminal_height = 24;

// Color codes
#define COLOR_RESET     "\033[0m"
#define COLOR_RED       "\033[31m"
#define COLOR_GREEN     "\033[32m"
#define COLOR_YELLOW    "\033[33m"
#define COLOR_BLUE      "\033[34m"
#define COLOR_MAGENTA   "\033[35m"
#define COLOR_CYAN      "\033[36m"
#define COLOR_WHITE     "\033[37m"
#define COLOR_BOLD      "\033[1m"

// Unicode characters
#define CHECK_MARK      "Unicode: \u2713"  // Will be replaced if not supported
#define CROSS_MARK      "Unicode: \u2717"  // Will be replaced if not supported
#define PROGRESS_CHARS  "Unicode: \u2588\u2588\u2588\u2588"  // Will be replaced if not supported

// Initialize UI system
bool ui_init(ui_mode_t mode)
{
    pthread_mutex_lock(&ui_mutex);
    
    // Initialize context
    memset(&ui_context, 0, sizeof(ui_context));
    ui_context.mode = mode;
    ui_context.verbosity_level = 1;
    
    // Detect terminal capabilities
    ui_detect_terminal_capabilities();
    
    // Load configuration
    ui_load_config();
    
    pthread_mutex_unlock(&ui_mutex);
    
    ui_log_event("UI initialized", "");
    return true;
}

// Cleanup UI system
void ui_cleanup(void)
{
    pthread_mutex_lock(&ui_mutex);
    
    // Stop animation
    ui_stop_animation();
    
    // Save configuration
    ui_save_config();
    
    // Reset terminal
    ui_reset_color();
    
    pthread_mutex_unlock(&ui_mutex);
    
    pthread_mutex_destroy(&ui_mutex);
}

// Detect terminal capabilities
static void ui_detect_terminal_capabilities(void)
{
    // Check for color support
    const char* term = getenv("TERM");
    const char* color_term = getenv("COLORTERM");
    
    supports_color = (term && strstr(term, "color")) || 
                    (color_term && strstr(color_term, "truecolor")) ||
                    (color_term && strstr(color_term, "24bit"));
    
    // Check for Unicode support
    const char* lang = getenv("LANG");
    supports_unicode = (lang && strstr(lang, "UTF-8"));
    
    // Get terminal size
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0) {
        terminal_width = w.ws_col;
        terminal_height = w.ws_row;
    }
    
    // Update context
    ui_context.color_enabled = supports_color;
    ui_context.unicode_enabled = supports_unicode;
}

// Clear screen
void ui_clear_screen(void)
{
    printf("\033[2J\033[H");
    fflush(stdout);
}

// Print header
void ui_print_header(const char* title)
{
    int title_len = strlen(title);
    int padding = (terminal_width - title_len - 4) / 2;
    
    if (ui_context.color_enabled) {
        ui_set_color(COLOR_CYAN);
    }
    
    printf("\n");
    printf("%.*s%s%.*s\n", padding, "==================================", 
           title, padding, "==================================");
    
    ui_reset_color();
    fflush(stdout);
}

// Print footer
void ui_print_footer(void)
{
    if (ui_context.color_enabled) {
        ui_set_color(COLOR_CYAN);
    }
    
    printf("%.*s\n", terminal_width, "==================================");
    
    ui_reset_color();
    fflush(stdout);
}

// Show main menu
int ui_show_main_menu(void)
{
    menu_t menu = {0};
    menu.type = MENU_TYPE_MAIN;
    strcpy(menu.title, "Rufus Linux - Menu Principal");
    menu.show_descriptions = true;
    menu.selected_item = 0;
    
    // Add menu items
    int item_count = 0;
    
    menu.items[item_count].id = 1;
    strcpy(menu.items[item_count].title, "Détecter les périphériques USB");
    strcpy(menu.items[item_count].description, "Scanner et lister les périphériques USB connectés");
    menu.items[item_count].enabled = true;
    menu.items[item_count].requires_device = false;
    menu.items[item_count].requires_iso = false;
    menu.items[item_count].shortcut = '1';
    item_count++;
    
    menu.items[item_count].id = 2;
    strcpy(menu.items[item_count].title, "Lister les périphériques de bloc");
    strcpy(menu.items[item_count].description, "Afficher tous les périphériques de bloc disponibles");
    menu.items[item_count].enabled = true;
    menu.items[item_count].requires_device = false;
    menu.items[item_count].requires_iso = false;
    menu.items[item_count].shortcut = '2';
    item_count++;
    
    menu.items[item_count].id = 3;
    strcpy(menu.items[item_count].title, "Écrire une image ISO");
    strcpy(menu.items[item_count].description, "Écrire une image ISO sur un périphérique USB");
    menu.items[item_count].enabled = true;
    menu.items[item_count].requires_device = true;
    menu.items[item_count].requires_iso = true;
    menu.items[item_count].shortcut = '3';
    item_count++;
    
    menu.items[item_count].id = 4;
    strcpy(menu.items[item_count].title, "Formater un périphérique");
    strcpy(menu.items[item_count].description, "Formater un périphérique avec un système de fichiers");
    menu.items[item_count].enabled = true;
    menu.items[item_count].requires_device = true;
    menu.items[item_count].requires_iso = false;
    menu.items[item_count].shortcut = '4';
    item_count++;
    
    menu.items[item_count].id = 5;
    strcpy(menu.items[item_count].title, "Vérifier l'intégrité");
    strcpy(menu.items[item_count].description, "Vérifier l'écriture d'une image ISO");
    menu.items[item_count].enabled = true;
    menu.items[item_count].requires_device = true;
    menu.items[item_count].requires_iso = true;
    menu.items[item_count].shortcut = '5';
    item_count++;
    
    menu.items[item_count].id = 6;
    strcpy(menu.items[item_count].title, "Sécurité et Logs");
    strcpy(menu.items[item_count].description, "Afficher les logs et les options de sécurité");
    menu.items[item_count].enabled = true;
    menu.items[item_count].requires_device = false;
    menu.items[item_count].requires_iso = false;
    menu.items[item_count].shortcut = '6';
    item_count++;
    
    menu.items[item_count].id = 7;
    strcpy(menu.items[item_count].title, "Paramètres");
    strcpy(menu.items[item_count].description, "Configurer les options du programme");
    menu.items[item_count].enabled = true;
    menu.items[item_count].requires_device = false;
    menu.items[item_count].requires_iso = false;
    menu.items[item_count].shortcut = '7';
    item_count++;
    
    menu.items[item_count].id = 8;
    strcpy(menu.items[item_count].title, "Aide");
    strcpy(menu.items[item_count].description, "Afficher l'aide et les informations");
    menu.items[item_count].enabled = true;
    menu.items[item_count].requires_device = false;
    menu.items[item_count].requires_iso = false;
    menu.items[item_count].shortcut = 'h';
    item_count++;
    
    menu.items[item_count].id = 9;
    strcpy(menu.items[item_count].title, "Quitter");
    strcpy(menu.items[item_count].description, "Quitter le programme");
    menu.items[item_count].enabled = true;
    menu.items[item_count].requires_device = false;
    menu.items[item_count].requires_iso = false;
    menu.items[item_count].shortcut = 'q';
    item_count++;
    
    menu.item_count = item_count;
    
    return ui_show_menu(&menu);
}

// Show menu and get selection
static int ui_show_menu(menu_t* menu)
{
    if (!menu) {
        return -1;
    }
    
    char input[MAX_INPUT_BUFFER];
    int selection = -1;
    
    while (selection == -1) {
        ui_clear_screen();
        ui_print_header(menu->title);
        
        // Display menu items
        for (int i = 0; i < menu->item_count; i++) {
            const menu_item_t* item = &menu->items[i];
            
            if (ui_context.color_enabled) {
                if (i == menu->selected_item) {
                    ui_set_color(COLOR_GREEN);
                    printf(" > ");
                } else {
                    printf("   ");
                }
            } else {
                printf(i == menu->selected_item ? " > " : "   ");
            }
            
            printf("%d. %s", item->id, item->title);
            
            if (item->shortcut) {
                printf(" (%c)", item->shortcut);
            }
            
            if (!item->enabled) {
                if (ui_context.color_enabled) {
                    ui_set_color(COLOR_RED);
                }
                printf(" [Désactivé]");
                ui_reset_color();
            }
            
            printf("\n");
            
            if (menu->show_descriptions && strlen(item->description) > 0) {
                printf("      %s\n", item->description);
            }
        }
        
        ui_print_footer();
        
        // Get user input
        printf("Sélectionnez une option (1-%d, ou 'q' pour quitter): ", menu->item_count);
        fflush(stdout);
        
        if (fgets(input, sizeof(input), stdin)) {
            // Remove newline
            input[strcspn(input, "\n")] = '\0';
            
            // Parse selection
            selection = ui_parse_menu_selection(input);
            
            if (selection == -2) {
                // User wants to quit
                return -1;
            } else if (selection == -1) {
                // Invalid selection
                ui_show_error("Sélection invalide", "Veuillez entrer un numéro valide");
                selection = -1; // Continue loop
            } else {
                // Valid selection
                return selection;
            }
        }
    }
    
    return selection;
}

// Parse menu selection
int ui_parse_menu_selection(const char* input)
{
    if (!input) {
        return -1;
    }
    
    // Check for quit
    if (strcmp(input, "q") == 0 || strcmp(input, "Q") == 0 || strcmp(input, "quit") == 0) {
        return -2;
    }
    
    // Check for help
    if (strcmp(input, "h") == 0 || strcmp(input, "H") == 0 || strcmp(input, "help") == 0) {
        return 8; // Help menu item
    }
    
    // Parse numeric selection
    char* endptr;
    long selection = strtol(input, &endptr, 10);
    
    if (endptr == input || *endptr != '\0') {
        return -1; // Not a number
    }
    
    if (selection < 1 || selection > 20) {
        return -1; // Out of range
    }
    
    return (int)selection;
}

// Show device selection menu
int ui_show_device_menu(const char* title, bool allow_refresh)
{
    ui_clear_screen();
    ui_print_header(title ? title : "Sélection d'un Périphérique");
    
    // Get device list
    disk_info_t disks[32];
    int disk_count = 0;
    
    if (!get_block_devices(disks, &disk_count, 32)) {
        ui_show_error("Erreur", "Impossible de lister les périphériques");
        return -1;
    }
    
    if (disk_count == 0) {
        ui_show_info("Aucun périphérique trouvé", "Aucun périphérique de bloc n'est disponible");
        return -1;
    }
    
    // Display devices
    printf("\n");
    for (int i = 0; i < disk_count; i++) {
        const disk_info_t* disk = &disks[i];
        
        if (ui_context.color_enabled) {
            if (disk->is_system_disk) {
                ui_set_color(COLOR_RED);
            } else if (disk->is_removable) {
                ui_set_color(COLOR_GREEN);
            } else {
                ui_set_color(COLOR_YELLOW);
            }
        }
        
        printf("[%d] %s\n", i + 1, disk->device_path);
        printf("    Taille: %.1f GB\n", (double)disk->size_bytes / (1024*1024*1024));
        
        if (strlen(disk->model) > 0) {
            printf("    Modèle: %s\n", disk->model);
        }
        
        if (strlen(disk->vendor) > 0) {
            printf("    Fabricant: %s\n", disk->vendor);
        }
        
        printf("    Type: %s", disk->is_removable ? "Amovible" : "Fixe");
        if (disk->is_system_disk) {
            printf(" [SYSTÈME]");
        }
        printf("\n");
        
        if (disk->mount_point_count > 0) {
            printf("    Monté: ");
            for (int j = 0; j < disk->mount_point_count; j++) {
                printf("%s ", disk->mount_points[j]);
            }
            printf("\n");
        }
        
        ui_reset_color();
        printf("\n");
    }
    
    ui_print_footer();
    
    // Get user selection
    char input[MAX_INPUT_BUFFER];
    printf("Sélectionnez un périphérique (1-%d): ", disk_count);
    fflush(stdout);
    
    if (fgets(input, sizeof(input), stdin)) {
        input[strcspn(input, "\n")] = '\0';
        
        char* endptr;
        long selection = strtol(input, &endptr, 10);
        
        if (endptr == input || *endptr != '\0' || selection < 1 || selection > disk_count) {
            ui_show_error("Sélection invalide", "Veuillez entrer un numéro valide");
            return -1;
        }
        
        return (int)selection - 1; // Return 0-based index
    }
    
    return -1;
}

// Show filesystem selection menu
int ui_show_filesystem_menu(void)
{
    ui_clear_screen();
    ui_print_header("Sélection d'un Système de Fichiers");
    
    // Get supported filesystems
    filesystem_info_t filesystems[FS_MAX];
    int fs_count = 0;
    
    if (!list_supported_filesystems(filesssystems, &fs_count, FS_MAX)) {
        ui_show_error("Erreur", "Aucun système de fichiers supporté trouvé");
        return -1;
    }
    
    // Display filesystems
    printf("\n");
    for (int i = 0; i < fs_count; i++) {
        const filesystem_info_t* fs = &filesystems[i];
        
        if (ui_context.color_enabled) {
            if (fs->is_native_linux) {
                ui_set_color(COLOR_GREEN);
            } else {
                ui_set_color(COLOR_CYAN);
            }
        }
        
        printf("[%d] %s\n", i + 1, fs->name);
        printf("    Taille: %.1f MB - %.1f %s\n", 
               (double)fs->min_size_bytes / (1024*1024),
               fs->max_size_bytes >= (1024ULL*1024*1024*1024) ? 
               (double)fs->max_size_bytes / (1024LL*1024*1024*1024) : 
               (double)fs->max_size_bytes / (1024*1024*1024),
               fs->max_size_bytes >= (1024ULL*1024*1024*1024) ? "TB" : "GB");
        
        printf("    Linux Natif: %s\n", fs->is_native_linux ? "Oui" : "Non");
        printf("    Journaling: %s\n", fs->supports_journaling ? "Oui" : "Non");
        printf("    Fichiers >4GB: %s\n", fs->supports_large_files ? "Oui" : "Non");
        
        ui_reset_color();
        printf("\n");
    }
    
    ui_print_footer();
    
    // Get user selection
    char input[MAX_INPUT_BUFFER];
    printf("Sélectionnez un système de fichiers (1-%d): ", fs_count);
    fflush(stdout);
    
    if (fgets(input, sizeof(input), stdin)) {
        input[strcspn(input, "\n")] = '\0';
        
        char* endptr;
        long selection = strtol(input, &endptr, 10);
        
        if (endptr == input || *endptr != '\0' || selection < 1 || selection > fs_count) {
            ui_show_error("Sélection invalide", "Veuillez entrer un numéro valide");
            return -1;
        }
        
        return (int)selection - 1; // Return 0-based index
    }
    
    return -1;
}

// Show ISO file selection
bool ui_select_iso_file(char* filepath, size_t filepath_size)
{
    ui_clear_screen();
    ui_print_header("Sélection d'une Image ISO");
    
    printf("\n");
    printf("Entrez le chemin complet vers le fichier ISO:\n");
    printf("Exemples:\n");
    printf("  /home/user/Downloads/ubuntu-22.04.iso\n");
    printf("  /tmp/windows10.iso\n");
    printf("  ~/isos/debian.iso\n");
    printf("\n");
    
    ui_print_footer();
    
    printf("Chemin du fichier ISO: ");
    fflush(stdout);
    
    if (fgets(filepath, filepath_size, stdin)) {
        filepath[strcspn(filepath, "\n")] = '\0';
        
        // Expand ~ to home directory
        if (filepath[0] == '~' && (filepath[1] == '/' || filepath[1] == '\0')) {
            char* home = getenv("HOME");
            if (home) {
                char expanded[filepath_size];
                snprintf(expanded, sizeof(expanded), "%s%s", home, filepath + 1);
                strncpy(filepath, expanded, filepath_size - 1);
                filepath[filepath_size - 1] = '\0';
            }
        }
        
        // Validate file exists
        struct stat st;
        if (stat(filepath, &st) != 0) {
            ui_show_error("Fichier introuvable", "Le fichier spécifié n'existe pas");
            return false;
        }
        
        if (!S_ISREG(st.st_mode)) {
            ui_show_error("Fichier invalide", "Le chemin spécifié n'est pas un fichier régulier");
            return false;
        }
        
        return true;
    }
    
    return false;
}

// Show confirmation dialog
bool ui_confirm_dialog(const char* title, const char* message, const char* details)
{
    ui_clear_screen();
    
    if (ui_context.color_enabled) {
        ui_set_color(COLOR_YELLOW);
    }
    
    ui_print_header(title);
    
    printf("\n");
    printf("%s\n", message);
    
    if (details && strlen(details) > 0) {
        printf("\n");
        printf("Détails:\n");
        printf("%s\n", details);
    }
    
    printf("\n");
    printf("AVERTISSEMENT: Cette opération peut détruire des données de manière irréversible!\n");
    printf("\n");
    
    ui_reset_color();
    
    ui_print_footer();
    
    char input[MAX_INPUT_BUFFER];
    printf("Confirmer l'opération? (tapez 'oui' pour continuer): ");
    fflush(stdout);
    
    if (fgets(input, sizeof(input), stdin)) {
        input[strcspn(input, "\n")] = '\0';
        
        return strcmp(input, "oui") == 0;
    }
    
    return false;
}

// Show progress bar
void ui_show_progress(const progress_info_t* progress)
{
    if (!progress) {
        return;
    }
    
    char progress_bar[PROGRESS_BAR_WIDTH + 1];
    ui_create_progress_bar(progress->percent, progress_bar, sizeof(progress_bar), true);
    
    printf("\r[%s] %.1f%%", progress_bar, progress->percent);
    
    if (strlen(progress->status) > 0) {
        printf(" %s", progress->status);
    }
    
    if (strlen(progress->time_remaining) > 0) {
        printf(" (reste: %s)", progress->time_remaining);
    }
    
    if (strlen(progress->speed) > 0) {
        printf(" (%s)", progress->speed);
    }
    
    fflush(stdout);
}

// Update progress
void ui_update_progress(uint64_t current, uint64_t total, const char* status)
{
    pthread_mutex_lock(&ui_mutex);
    
    ui_context.progress.current = current;
    ui_context.progress.total = total;
    ui_context.progress.percent = (float)current * 100.0f / total;
    
    if (status) {
        strncpy(ui_context.progress.status, status, MAX_STATUS_TEXT - 1);
        ui_context.progress.status[MAX_STATUS_TEXT - 1] = '\0';
    }
    
    // Calculate time remaining and speed
    static time_t start_time = 0;
    static bool first_update = true;
    
    if (first_update) {
        start_time = time(NULL);
        first_update = false;
    }
    
    time_t current_time = time(NULL);
    double elapsed = difftime(current_time, start_time);
    
    if (elapsed > 0 && current > 0) {
        double bytes_per_second = current / elapsed;
        
        // Format speed
        ui_format_speed(bytes_per_second, ui_context.progress.speed, sizeof(ui_context.progress.speed));
        
        // Calculate ETA
        uint64_t remaining = total - current;
        double eta_seconds = remaining / bytes_per_second;
        ui_format_time((uint64_t)eta_seconds, ui_context.progress.time_remaining, sizeof(ui_context.progress.time_remaining));
    }
    
    ui_show_progress(&ui_context.progress);
    
    pthread_mutex_unlock(&ui_mutex);
}

// Create progress bar string
void ui_create_progress_bar(float percent, char* buffer, size_t buffer_size, bool animated)
{
    if (!buffer || buffer_size < PROGRESS_BAR_WIDTH + 1) {
        return;
    }
    
    int filled = (int)(percent * PROGRESS_BAR_WIDTH / 100.0f);
    int empty = PROGRESS_BAR_WIDTH - filled;
    
    if (ui_context.unicode_enabled && animated) {
        // Use Unicode block characters
        for (int i = 0; i < filled; i++) {
            buffer[i] = 0x2588; // Full block
        }
        for (int i = filled; i < PROGRESS_BAR_WIDTH; i++) {
            buffer[i] = 0x2591; // Light shade
        }
    } else {
        // Use ASCII characters
        for (int i = 0; i < filled; i++) {
            buffer[i] = '=';
        }
        for (int i = filled; i < PROGRESS_BAR_WIDTH; i++) {
            buffer[i] = '-';
        }
    }
    
    buffer[PROGRESS_BAR_WIDTH] = '\0';
}

// Format bytes for display
void ui_format_bytes(uint64_t bytes, char* buffer, size_t buffer_size)
{
    const char* units[] = {"B", "KB", "MB", "GB", "TB", "PB"};
    int unit_index = 0;
    double size = (double)bytes;
    
    while (size >= 1024.0 && unit_index < 5) {
        size /= 1024.0;
        unit_index++;
    }
    
    snprintf(buffer, buffer_size, "%.1f %s", size, units[unit_index]);
}

// Format time for display
void ui_format_time(uint64_t seconds, char* buffer, size_t buffer_size)
{
    if (seconds < 60) {
        snprintf(buffer, buffer_size, "%llu sec", (unsigned long long)seconds);
    } else if (seconds < 3600) {
        uint64_t minutes = seconds / 60;
        uint64_t secs = seconds % 60;
        snprintf(buffer, buffer_size, "%llu min %llu sec", (unsigned long long)minutes, (unsigned long long)secs);
    } else {
        uint64_t hours = seconds / 3600;
        uint64_t minutes = (seconds % 3600) / 60;
        snprintf(buffer, buffer_size, "%llu h %llu min", (unsigned long long)hours, (unsigned long long)minutes);
    }
}

// Format speed for display
void ui_format_speed(double bytes_per_second, char* buffer, size_t buffer_size)
{
    const char* units[] = {"B/s", "KB/s", "MB/s", "GB/s"};
    int unit_index = 0;
    double speed = bytes_per_second;
    
    while (speed >= 1024.0 && unit_index < 3) {
        speed /= 1024.0;
        unit_index++;
    }
    
    snprintf(buffer, buffer_size, "%.1f %s", speed, units[unit_index]);
}

// Show success message
void ui_show_success(const char* message, const char* details)
{
    if (ui_context.color_enabled) {
        ui_set_color(COLOR_GREEN);
    }
    
    printf("\n");
    printf("=== SUCCÈS ===\n");
    printf("%s\n", message);
    
    if (details && strlen(details) > 0) {
        printf("\n%s\n", details);
    }
    
    printf("\n");
    
    ui_reset_color();
    fflush(stdout);
}

// Show error message
void ui_show_error(const char* message, const char* details)
{
    if (ui_context.color_enabled) {
        ui_set_color(COLOR_RED);
    }
    
    printf("\n");
    printf("=== ERREUR ===\n");
    printf("%s\n", message);
    
    if (details && strlen(details) > 0) {
        printf("\n%s\n", details);
    }
    
    printf("\n");
    
    ui_reset_color();
    fflush(stdout);
}

// Show warning message
void ui_show_warning(const char* message, const char* details)
{
    if (ui_context.color_enabled) {
        ui_set_color(COLOR_YELLOW);
    }
    
    printf("\n");
    printf("=== AVERTISSEMENT ===\n");
    printf("%s\n", message);
    
    if (details && strlen(details) > 0) {
        printf("\n%s\n", details);
    }
    
    printf("\n");
    
    ui_reset_color();
    fflush(stdout);
}

// Show info message
void ui_show_info(const char* message, const char* details)
{
    if (ui_context.color_enabled) {
        ui_set_color(COLOR_CYAN);
    }
    
    printf("\n");
    printf("=== INFORMATION ===\n");
    printf("%s\n", message);
    
    if (details && strlen(details) > 0) {
        printf("\n%s\n", details);
    }
    
    printf("\n");
    
    ui_reset_color();
    fflush(stdout);
}

// Set color
void ui_set_color(int color)
{
    if (ui_context.color_enabled) {
        printf("%s", color);
    }
}

// Reset color
void ui_reset_color(void)
{
    if (ui_context.color_enabled) {
        printf("%s", COLOR_RESET);
    }
}

// Interactive mode main loop
int ui_interactive_main_loop(void)
{
    int choice;
    
    while (true) {
        choice = ui_show_main_menu();
        
        switch (choice) {
            case 1: // Detect USB devices
                ui_show_device_list("Périphériques USB");
                break;
                
            case 2: // List block devices
                ui_show_device_list("Périphériques de Bloc");
                break;
                
            case 3: // Write ISO
                ui_handle_write_iso();
                break;
                
            case 4: // Format device
                ui_handle_format_device();
                break;
                
            case 5: // Verify
                ui_handle_verify();
                break;
                
            case 6: // Security and logs
                ui_handle_security_logs();
                break;
                
            case 7: // Settings
                ui_show_settings_menu();
                break;
                
            case 8: // Help
                ui_show_help();
                break;
                
            case 9: // Quit
            case -1: // User pressed 'q'
                if (ui_confirm_dialog("Quitter", "Voulez-vous vraiment quitter Rufus Linux?", "")) {
                    return 0;
                }
                break;
                
            default:
                ui_show_error("Option invalide", "Veuillez sélectionner une option valide");
                break;
        }
        
        if (choice != -1) {
            printf("\nAppuyez sur Entrée pour continuer...");
            getchar();
        }
    }
    
    return 0;
}

// Handle write ISO operation
static void ui_handle_write_iso(void)
{
    // Select device
    int device_index = ui_show_device_menu("Sélection du Périphérique Cible", true);
    if (device_index < 0) {
        return;
    }
    
    disk_info_t disks[32];
    int disk_count = 0;
    
    if (!get_block_devices(disks, &disk_count, 32) || device_index >= disk_count) {
        ui_show_error("Erreur", "Impossible d'obtenir les informations du périphérique");
        return;
    }
    
    const char* device_path = disks[device_index].device_path;
    
    // Select ISO file
    char iso_path[MAX_INPUT_BUFFER];
    if (!ui_select_iso_file(iso_path, sizeof(iso_path))) {
        return;
    }
    
    // Show confirmation
    char details[1024];
    snprintf(details, sizeof(details), 
             "Périphérique: %s\nTaille: %.1f GB\nImage: %s\n\nCette opération effacera complètement le périphérique!",
             device_path, (double)disks[device_index].size_bytes / (1024*1024*1024), iso_path);
    
    if (!ui_confirm_dialog("Écrire l'Image ISO", "Voulez-vous écrire cette image ISO sur le périphérique sélectionné?", details)) {
        return;
    }
    
    // Perform write operation using existing write functionality
    iso_info_t iso_info;
    if (!get_iso_info(iso_path, &iso_info)) {
        ui_show_error("Fichier ISO invalide", "Impossible de lire les informations de l'image ISO");
        return;
    }
    
    // Integrate with existing write functionality
    write_progress_t progress;
    write_result_t result = write_iso_to_device(iso_path, device_path, &progress);
    
    if (result == WRITE_SUCCESS) {
        ui_show_success("Écriture terminée", "L'image ISO a été écrite avec succès sur le périphérique");
    } else {
        ui_show_error("Écriture échouée", "L'écriture de l'image ISO a échoué");
    }
}

// Handle format device operation
static void ui_handle_format_device(void)
{
    // Select device
    int device_index = ui_show_device_menu("Sélection du Périphérique à Formater", true);
    if (device_index < 0) {
        return;
    }
    
    disk_info_t disks[32];
    int disk_count = 0;
    
    if (!get_block_devices(disks, &disk_count, 32) || device_index >= disk_count) {
        ui_show_error("Erreur", "Impossible d'obtenir les informations du périphérique");
        return;
    }
    
    const char* device_path = disks[device_index].device_path;
    
    // Select filesystem
    int fs_index = ui_show_filesystem_menu();
    if (fs_index < 0) {
        return;
    }
    
    filesystem_info_t filesystems[FS_MAX];
    int fs_count = 0;
    
    if (!list_supported_filesystems(filesssystems, &fs_count, FS_MAX) || fs_index >= fs_count) {
        ui_show_error("Erreur", "Impossible d'obtenir les informations du système de fichiers");
        return;
    }
    
    // Get label
    char label[MAX_INPUT_BUFFER];
    ui_input_dialog("Label du Système de Fichiers", "Entrez un label (optionnel): ", label, sizeof(label));
    
    // Show confirmation
    char details[1024];
    snprintf(details, sizeof(details), 
             "Périphérique: %s\nTaille: %.1f GB\nSystème: %s\nLabel: %s\n\nCette opération effacera complètement le périphérique!",
             device_path, (double)disks[device_index].size_bytes / (1024*1024*1024), 
             filesystems[fs_index].name, strlen(label) > 0 ? label : "Aucun");
    
    if (!ui_confirm_dialog("Formater le Périphérique", "Voulez-vous formater ce périphérique?", details)) {
        return;
    }
    
    // Perform format operation using existing format functionality
    filesystem_type_t fs_type = (filesystem_type_t)fs_index;
    bool format_result = format_device(device_path, fs_type, strlen(label) > 0 ? label : NULL, PARTITION_SCHEME_MBR, true);
    
    if (format_result) {
        ui_show_success("Formatage terminé", "Le périphérique a été formaté avec succès");
    } else {
        ui_show_error("Formatage échoué", "Le formatage du périphérique a échoué");
    }
}

// Handle verify operation
static void ui_handle_verify(void)
{
    // Select device
    int device_index = ui_show_device_menu("Sélection du Périphérique à Vérifier", true);
    if (device_index < 0) {
        return;
    }
    
    disk_info_t disks[32];
    int disk_count = 0;
    
    if (!get_block_devices(disks, &disk_count, 32) || device_index >= disk_count) {
        ui_show_error("Erreur", "Impossible d'obtenir les informations du périphérique");
        return;
    }
    
    const char* device_path = disks[device_index].device_path;
    
    // Select ISO file
    char iso_path[MAX_INPUT_BUFFER];
    if (!ui_select_iso_file(iso_path, sizeof(iso_path))) {
        return;
    }
    
    // Show confirmation
    char details[1024];
    snprintf(details, sizeof(details), 
             "Périphérique: %s\nImage: %s\n\nCette opération vérifiera l'intégrité de l'écriture.",
             device_path, iso_path);
    
    if (!ui_confirm_dialog("Vérifier l'Écriture", "Voulez-vous vérifier l'intégrité de l'écriture?", details)) {
        return;
    }
    
    // Perform verify operation using existing verify functionality
    verification_progress_t progress;
    verify_result_t result = verify_iso_write(iso_path, device_path, &progress);
    
    if (result == VERIFY_SUCCESS) {
        ui_show_success("Vérification terminée", "L'intégrité de l'écriture a été vérifiée avec succès");
    } else {
        ui_show_error("Vérification échouée", "La vérification de l'intégrité a échoué");
    }
}

// Handle security and logs
static void ui_handle_security_logs(void)
{
    menu_t menu = {0};
    menu.type = MENU_TYPE_SECURITY;
    strcpy(menu.title, "Sécurité et Logs");
    menu.show_descriptions = true;
    menu.selected_item = 0;
    
    int item_count = 0;
    
    menu.items[item_count].id = 1;
    strcpy(menu.items[item_count].title, "Afficher les logs récents");
    strcpy(menu.items[item_count].description, "Afficher les 20 derniers logs");
    menu.items[item_count].enabled = true;
    menu.items[item_count].shortcut = '1';
    item_count++;
    
    menu.items[item_count].id = 2;
    strcpy(menu.items[item_count].title, "Afficher l'audit");
    strcpy(menu.items[item_count].description, "Afficher l'audit des opérations");
    menu.items[item_count].enabled = true;
    menu.items[item_count].shortcut = '2';
    item_count++;
    
    menu.items[item_count].id = 3;
    strcpy(menu.items[item_count].title, "Activer le monitoring de sécurité");
    strcpy(menu.items[item_count].description, "Activer le monitoring de sécurité avancée");
    menu.items[item_count].enabled = true;
    menu.items[item_count].shortcut = '3';
    item_count++;
    
    menu.items[item_count].id = 4;
    strcpy(menu.items[item_count].title, "Vérifier la sécurité d'un périphérique");
    strcpy(menu.items[item_count].description, "Effectuer une vérification de sécurité complète");
    menu.items[item_count].enabled = true;
    menu.items[item_count].shortcut = '4';
    item_count++;
    
    menu.item_count = item_count;
    
    int choice = ui_show_menu(&menu);
    
    switch (choice) {
        case 1:
            ui_clear_screen();
            ui_print_header("Logs Récents");
            print_recent_logs(20);
            break;
            
        case 2:
            ui_clear_screen();
            ui_print_header("Audit des Opérations");
            print_recent_audit(20);
            break;
            
        case 3:
            enable_security_monitoring();
            ui_show_success("Monitoring activé", "Le monitoring de sécurité a été activé");
            break;
            
        case 4:
            {
                int device_index = ui_show_device_menu("Sélection du Périphérique à Vérifier", true);
                if (device_index >= 0) {
                    disk_info_t disks[32];
                    int disk_count = 0;
                    if (get_block_devices(disks, &disk_count, 32) && device_index < disk_count) {
                        if (enhanced_device_safety_check(disks[device_index].device_path)) {
                            ui_show_success("Périphérique sécurisé", "Le périphérique passe toutes les vérifications de sécurité");
                        } else {
                            ui_show_error("Périphérique non sécurisé", "Le périphérique a échoué aux vérifications de sécurité");
                        }
                    }
                }
            }
            break;
    }
}

// Show device list
void ui_show_device_list(const char* title)
{
    ui_show_device_menu(title, false);
}

// Show help
void ui_show_help(void)
{
    ui_clear_screen();
    ui_print_header("Aide - Rufus Linux");
    
    printf("\n");
    printf("Rufus Linux est un utilitaire de boot USB pour Linux.\n");
    printf("\n");
    printf("Fonctionnalités principales:\n");
    printf("  - Détection de périphériques USB\n");
    printf("  - Écriture d'images ISO sur USB\n");
    printf("  - Formatage de périphériques\n");
    printf("  - Vérification d'intégrité\n");
    printf("  - Sécurité avancée et logging\n");
    printf("\n");
    printf("Modules implémentés:\n");
    printf("  1. Détection USB (libusb-1.0)\n");
    printf("  2. Sélection sécurisée de périphériques\n");
    printf("  3. Écriture ISO haute performance\n");
    printf("  4. Vérification cryptographique\n");
    printf("  5. Sécurité avancée et audit\n");
    printf("  6. Support multi-systèmes de fichiers\n");
    printf("  7. Interface utilisateur interactive\n");
    printf("\n");
    printf("Pour plus d'informations, consultez la documentation.\n");
    
    ui_print_footer();
}

// Load configuration
bool ui_load_config(void)
{
    // This would load configuration from a file
    // For now, use defaults
    ui_context.color_enabled = supports_color;
    ui_context.unicode_enabled = supports_unicode;
    ui_context.animations_enabled = true;
    
    return true;
}

// Save configuration
bool ui_save_config(void)
{
    // This would save configuration to a file
    return true;
}
