/*
 * Rufus Linux - Main CLI Application
 * Copyright © 2026 Port Project
 *
 * Main entry point for USB detection and formatting
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <time.h>
#include "usb_detect.h"
#include "disk_access.h"
#include "iso_write.h"
#include "verification.h"
#include "security.h"
#include "filesystem.h"
#include "ui.h"

#define VERSION "0.1.0"

static void print_usage(const char* program_name)
{
    printf("Rufus Linux v%s - Utilitaire de Boot USB\n", VERSION);
    printf("Usage: %s [OPTIONS]\n\n", program_name);
    printf("OPTIONS:\n");
    printf("  -h, --help           Affiche cette aide\n");
    printf("  -v, --version        Affiche la version\n");
    printf("  -l, --list-usb       Liste les périphériques USB\n");
    printf("  -s, --list-storage   Liste uniquement les périphériques de stockage\n");
    printf("  -L, --list-disks     Liste les périphériques de bloc\n");
    printf("  -c, --check-device   Vérifie la sécurité d'un périphérique\n");
    printf("  -d, --device DEV     Spécifie le périphérique cible (ex: /dev/sdb)\n");
    printf("  -i, --image ISO      Spécifie l'image ISO à écrire\n");
    printf("  -w, --write-iso      Écrit l'image ISO sur le périphérique\n");
    printf("  -V, --verify        Vérifie l'intégrité de l'écriture\n");
    printf("  -S, --security      Active le monitoring de sécurité avancée\n");
    printf("  --logs              Affiche les logs récents\n");
    printf("  --audit             Affiche l'audit des opérations\n");
    printf("  -f, --format FS     Formate le périphérique avec le système de fichiers\n");
    printf("  --list-fs           Liste les systèmes de fichiers supportés\n");
    printf("  -I, --interactive   Mode interface interactive\n");
    printf("  -b, --backup-mbr     Crée un backup MBR du périphérique\n");
    printf("\n");
    printf("EXEMPLES:\n");
    printf("  %s --list-usb                    # Liste tous les périphériques USB\n", program_name);
    printf("  %s --list-storage                # Liste les périphériques de stockage\n", program_name);
    printf("  %s --list-disks                  # Liste les périphériques de bloc\n", program_name);
    printf("  %s --check-device /dev/sdb       # Vérifie la sécurité du périphérique\n", program_name);
    printf("  %s --backup-mbr /dev/sdb         # Crée un backup MBR\n", program_name);
    printf("  %s --write-iso --device /dev/sdb --image ubuntu.iso  # Écrit une ISO\n", program_name);
    printf("  %s --verify --device /dev/sdb --image ubuntu.iso  # Vérifie l'écriture\n", program_name);
    printf("  %s --security --write-iso --device /dev/sdb --image ubuntu.iso  # Mode sécurité\n", program_name);
    printf("  %s --format fat32 --device /dev/sdb --label USBSTICK    # Formatage FAT32\n", program_name);
    printf("  %s --list-fs                      # Liste les systèmes de fichiers\n", program_name);
    printf("  %s --interactive                   # Mode interface interactive\n", program_name);
    printf("  %s --logs                         # Affiche les logs récents\n", program_name);
    printf("  %s --audit                        # Affiche l'audit des opérations\n", program_name);
    printf("\n");
    printf("ATTENTION: Ce programme peut détruire des données. Utilisez avec précaution.\n");
}

static void print_version(void)
{
    printf("Rufus Linux v%s\n", VERSION);
    printf("Port Linux de Rufus - Utilitaire de Boot USB\n");
    printf("Copyright © 2026 Port Project\n");
}

int main(int argc, char* argv[])
{
    int opt;
    int list_usb = 0;
    int list_storage = 0;
    int list_disks = 0;
    int check_device = 0;
    int backup_mbr = 0;
    int write_iso = 0;
    int verify = 0;
    int security_mode = 0;
    int show_logs = 0;
    int show_audit = 0;
    int format_device = 0;
    int list_filesystems = 0;
    int interactive_mode = 0;
    char* device_path = NULL;
    char* image_path = NULL;
    char* filesystem_type = NULL;
    char* filesystem_label = NULL;
    
    static struct option long_options[] = {
        {"help",        no_argument,       0, 'h'},
        {"version",     no_argument,       0, 'v'},
        {"list-usb",    no_argument,       0, 'l'},
        {"list-storage",no_argument,       0, 's'},
        {"list-disks",  no_argument,       0, 'L'},
        {"check-device",no_argument,       0, 'c'},
        {"backup-mbr",  no_argument,       0, 'b'},
        {"write-iso",   no_argument,       0, 'w'},
        {"verify",      no_argument,       0, 'V'},
        {"security",    no_argument,       0, 'S'},
        {"logs",        no_argument,       0, 1001},
        {"audit",       no_argument,       0, 1002},
        {"format",      required_argument, 0, 'f'},
        {"list-fs",     no_argument,       0, 1003},
        {"label",       required_argument, 0, 1004},
        {"interactive", no_argument,       0, 'I'},
        {"device",      required_argument, 0, 'd'},
        {"image",       required_argument, 0, 'i'},
        {0, 0, 0, 0}
    };
    
    while ((opt = getopt_long(argc, argv, "hvlsLcbd:wi:VSf:I", long_options, NULL)) != -1) {
        switch (opt) {
            case 'h':
                print_usage(argv[0]);
                return 0;
            case 'v':
                print_version();
                return 0;
            case 'l':
                list_usb = 1;
                break;
            case 's':
                list_storage = 1;
                break;
            case 'L':
                list_disks = 1;
                break;
            case 'c':
                check_device = 1;
                break;
            case 'b':
                backup_mbr = 1;
                break;
            case 'w':
                write_iso = 1;
                break;
            case 'V':
                verify = 1;
                break;
            case 'S':
                security_mode = 1;
                break;
            case 1001:
                show_logs = 1;
                break;
            case 1002:
                show_audit = 1;
                break;
            case 'f':
                format_device = 1;
                filesystem_type = optarg;
                break;
            case 1003:
                list_filesystems = 1;
                break;
            case 1004:
                filesystem_label = optarg;
                break;
            case 'I':
                interactive_mode = 1;
                break;
            case 'd':
                device_path = optarg;
                break;
            case 'i':
                image_path = optarg;
                break;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }
    
    // Initialize disk access
    if (!disk_access_init()) {
        fprintf(stderr, "Erreur: impossible d'initialiser l'accès disque\n");
        return 1;
    }
    
    // Initialize ISO writing
    if (!iso_write_init()) {
        fprintf(stderr, "Erreur: impossible d'initialiser l'écriture ISO\n");
        disk_access_cleanup();
        return 1;
    }
    
    // Initialize verification
    if (!verification_init()) {
        fprintf(stderr, "Erreur: impossible d'initialiser la vérification\n");
        disk_access_cleanup();
        iso_write_cleanup();
        return 1;
    }
    
    // Initialize security
    if (!security_init()) {
        fprintf(stderr, "Erreur: impossible d'initialiser la sécurité\n");
        disk_access_cleanup();
        iso_write_cleanup();
        verification_cleanup();
        return 1;
    }
    
    // Initialize filesystem
    if (!filesystem_init()) {
        fprintf(stderr, "Erreur: impossible d'initialiser le système de fichiers\n");
        disk_access_cleanup();
        iso_write_cleanup();
        verification_cleanup();
        security_cleanup();
        return 1;
    }
    
    // Initialize UI
    ui_mode_t ui_mode = interactive_mode ? UI_MODE_INTERACTIVE : UI_MODE_CLI;
    if (!ui_init(ui_mode)) {
        fprintf(stderr, "Erreur: impossible d'initialiser l'interface utilisateur\n");
        disk_access_cleanup();
        iso_write_cleanup();
        verification_cleanup();
        security_cleanup();
        filesystem_cleanup();
        return 1;
    }
    
    // Handle interactive mode
    if (interactive_mode) {
        int result = ui_interactive_main_loop();
        
        // Cleanup
        ui_cleanup();
        disk_access_cleanup();
        iso_write_cleanup();
        verification_cleanup();
        security_cleanup();
        filesystem_cleanup();
        
        return result;
    }
    
    // Initialize libusb for CLI mode
    libusb_context* ctx = usb_init();
    if (!ctx) {
        fprintf(stderr, "Erreur: impossible d'initialiser libusb\n");
        disk_access_cleanup();
        iso_write_cleanup();
        verification_cleanup();
        security_cleanup();
        filesystem_cleanup();
        ui_cleanup();
        return 1;
    }
    
    // Scan USB devices
    usb_device_list_t device_list;
    int device_count = usb_scan_devices(ctx, &device_list);
    if (device_count < 0) {
        fprintf(stderr, "Erreur: impossible de scanner les périphériques USB\n");
        usb_cleanup(ctx);
        ui_cleanup();
        return 1;
    }
    
    // Handle different modes
    if (list_usb || list_storage) {
        if (list_storage) {
            printf("=== Périphériques de Stockage USB ===\n");
            int storage_count = 0;
            for (int i = 0; i < device_list.count; i++) {
                if (device_list.devices[i].is_storage_device) {
                    printf("[%d] Bus %03d Device %03d: %04x:%04x - %s %s\n", 
                           ++storage_count,
                           device_list.devices[i].bus_number,
                           device_list.devices[i].device_address,
                           device_list.devices[i].vendor_id,
                           device_list.devices[i].product_id,
                           device_list.devices[i].manufacturer,
                           device_list.devices[i].product);
                }
            }
            if (storage_count == 0) {
                printf("Aucun périphérique de stockage trouvé\n");
            }
        } else {
            usb_print_device_list(&device_list);
        }
    }
    
    // List block devices
    if (list_disks) {
        printf("=== Périphériques de Bloc ===\n");
        disk_info_t disks[32];
        int disk_count = 0;
        
        if (get_block_devices(disks, &disk_count, 32)) {
            for (int i = 0; i < disk_count; i++) {
                printf("[%d] %s - %.1f GB - %s %s\n", 
                       i + 1,
                       disks[i].device_path,
                       (double)disks[i].size_bytes / (1024.0*1024.0*1024.0),
                       disks[i].is_removable ? "[Amovible]" : "[Fixe]",
                       disks[i].is_system_disk ? "[SYSTÈME]" : "");
            }
        } else {
            printf("Erreur: impossible de lister les périphériques de bloc\n");
        }
    }
    
    // Check device safety
    if (check_device) {
        if (!device_path) {
            fprintf(stderr, "Erreur: --check-device nécessite --device DEV\n");
            usb_cleanup(ctx);
            disk_access_cleanup();
            return 1;
        }
        
        printf("=== Vérification de Sécurité ===\n");
        disk_info_t disk_info;
        
        if (get_disk_info(device_path, &disk_info)) {
            print_disk_info(&disk_info);
            print_safety_warnings(&disk_info);
            
            if (is_safe_device(&disk_info)) {
                printf("\nRésultat: SECURISÉ pour utilisation\n");
            } else {
                printf("\nRésultat: NON SÉCURISÉ - Ne pas utiliser!\n");
            }
        } else {
            fprintf(stderr, "Erreur: impossible d'obtenir les informations du périphérique\n");
        }
    }
    
    // Backup MBR
    if (backup_mbr) {
        if (!device_path) {
            fprintf(stderr, "Erreur: --backup-mbr nécessite --device DEV\n");
            usb_cleanup(ctx);
            disk_access_cleanup();
            return 1;
        }
        
        printf("=== Backup MBR ===\n");
        disk_info_t disk_info;
        mbr_backup_t backup;
        char backup_filename[512];
        
        if (get_disk_info(device_path, &disk_info)) {
            print_disk_info(&disk_info);
            
            if (get_user_confirmation("Créer un backup MBR", &disk_info)) {
                if (create_mbr_backup(device_path, &backup)) {
                    snprintf(backup_filename, sizeof(backup_filename), 
                            "mbr_backup_%s_%ld.bin", 
                            strrchr(device_path, '/') + 1,
                            time(NULL));
                    
                    if (save_mbr_backup_to_file(&backup, backup_filename)) {
                        printf("Backup MBR créé avec succès: %s\n", backup_filename);
                    } else {
                        fprintf(stderr, "Erreur: impossible de sauvegarder le backup\n");
                    }
                } else {
                    fprintf(stderr, "Erreur: impossible de créer le backup MBR\n");
                }
            } else {
                printf("Backup MBR annulé\n");
            }
        } else {
            fprintf(stderr, "Erreur: impossible d'obtenir les informations du périphérique\n");
        }
    }
    
    // Write ISO operation
    if (write_iso) {
        if (!device_path || !image_path) {
            fprintf(stderr, "Erreur: --write-iso nécessite --device DEV et --image ISO\n");
            usb_cleanup(ctx);
            disk_access_cleanup();
            iso_write_cleanup();
            return 1;
        }
        
        printf("=== Écriture ISO ===\n");
        
        // Validate ISO file
        iso_info_t iso_info;
        if (!get_iso_info(image_path, &iso_info)) {
            fprintf(stderr, "Erreur: Fichier ISO invalide\n");
            usb_cleanup(ctx);
            disk_access_cleanup();
            iso_write_cleanup();
            return 1;
        }
        
        print_iso_info(&iso_info);
        
        // Get device info and validate safety
        disk_info_t disk_info;
        if (!get_disk_info(device_path, &disk_info)) {
            fprintf(stderr, "Erreur: impossible d'obtenir les informations du périphérique\n");
            usb_cleanup(ctx);
            disk_access_cleanup();
            iso_write_cleanup();
            return 1;
        }
        
        print_disk_info(&disk_info);
        
        // Safety checks
        if (!is_safe_device(&disk_info)) {
            fprintf(stderr, "Erreur: Périphérique non sécurisé pour l'écriture\n");
            usb_cleanup(ctx);
            disk_access_cleanup();
            iso_write_cleanup();
            return 1;
        }
        
        // Get user confirmation
        if (!get_user_confirmation("Écrire l'image ISO", &disk_info)) {
            printf("Écriture ISO annulée\n");
            usb_cleanup(ctx);
            disk_access_cleanup();
            iso_write_cleanup();
            return 0;
        }
        
        // Create MBR backup before writing
        mbr_backup_t backup;
        char backup_filename[512];
        bool backup_created = false;
        
        if (create_mbr_backup(device_path, &backup)) {
            snprintf(backup_filename, sizeof(backup_filename), 
                    "mbr_backup_%s_%ld.bin", 
                    strrchr(device_path, '/') + 1,
                    time(NULL));
            
            if (save_mbr_backup_to_file(&backup, backup_filename)) {
                printf("Backup MBR créé: %s\n", backup_filename);
                backup_created = true;
            }
        }
        
        // Unmount device if mounted
        if (is_device_mounted(device_path)) {
            printf("Démontage du périphérique...\n");
            if (!unmount_device(device_path)) {
                fprintf(stderr, "AVERTISSEMENT: Démontage partiel ou échoué\n");
            }
        }
        
        // Audit write start
        audit_action(AUDIT_ACTION_WRITE_START, device_path, image_path, 0, false, "Starting ISO write");
        
        // Write ISO
        write_progress_t progress;
        write_result_t result = write_iso_to_device(image_path, device_path, &progress);
        
        if (result == WRITE_SUCCESS) {
            printf("\n=== ÉCRITURE TERMINÉE AVEC SUCCÈS ===\n");
            
            // Audit write success
            audit_action(AUDIT_ACTION_WRITE_SUCCESS, device_path, image_path, iso_info.size_bytes, true, "ISO write completed successfully");
            
            if (backup_created) {
                printf("Backup MBR disponible: %s\n", backup_filename);
                printf("Pour restaurer: rufus-linux --restore-mbr %s --device %s\n", 
                       backup_filename, device_path);
            }
        } else {
            printf("\n=== ÉCRITURE ÉCHOUÉE ===\n");
            
            // Audit write failure
            audit_action(AUDIT_ACTION_WRITE_FAILED, device_path, image_path, 0, false, "ISO write failed");
            
            switch (result) {
                case WRITE_ERROR_CANCELLED:
                    printf("Opération annulée par l'utilisateur\n");
                    break;
                case WRITE_ERROR_INVALID_ISO:
                    printf("Fichier ISO invalide\n");
                    break;
                case WRITE_ERROR_INVALID_DEVICE:
                    printf("Périphérique invalide\n");
                    break;
                case WRITE_ERROR_SPACE:
                    printf("Espace insuffisant sur le périphérique\n");
                    break;
                default:
                    printf("Erreur during l'écriture (code: %d)\n", result);
                    break;
            }
            
            // Offer to restore MBR if backup was created
            if (backup_created) {
                printf("\nRestaurer le MBR depuis le backup? (o/n): ");
                char response[10];
                if (fgets(response, sizeof(response), stdin) && 
                    (response[0] == 'o' || response[0] == 'O')) {
                    if (restore_mbr_backup(device_path, &backup)) {
                        printf("MBR restauré avec succès\n");
                    } else {
                        printf("Erreur lors de la restauration du MBR\n");
                    }
                }
            }
        }
    }
    
    // Verify ISO write
    if (verify) {
        if (!device_path || !image_path) {
            fprintf(stderr, "Erreur: --verify nécessite --device DEV et --image ISO\n");
            usb_cleanup(ctx);
            disk_access_cleanup();
            iso_write_cleanup();
            verification_cleanup();
            return 1;
        }
        
        printf("=== Vérification de l'Écriture ===\n");
        
        // Validate ISO file
        iso_info_t iso_info;
        if (!get_iso_info(image_path, &iso_info)) {
            fprintf(stderr, "Erreur: Fichier ISO invalide\n");
            usb_cleanup(ctx);
            disk_access_cleanup();
            iso_write_cleanup();
            verification_cleanup();
            return 1;
        }
        
        print_iso_info(&iso_info);
        
        // Get device info
        disk_info_t disk_info;
        if (!get_disk_info(device_path, &disk_info)) {
            fprintf(stderr, "Erreur: impossible d'obtenir les informations du périphérique\n");
            usb_cleanup(ctx);
            disk_access_cleanup();
            iso_write_cleanup();
            verification_cleanup();
            return 1;
        }
        
        print_disk_info(&disk_info);
        
        // Check if device has enough space for verification
        if (disk_info.size_bytes < iso_info.size_bytes) {
            fprintf(stderr, "Erreur: Le périphérique est plus petit que l'ISO\n");
            fprintf(stderr, "  Taille ISO: %.1f GB\n", (double)iso_info.size_bytes / (1024*1024*1024));
            fprintf(stderr, "  Taille périphérique: %.1f GB\n", (double)disk_info.size_bytes / (1024*1024*1024));
            usb_cleanup(ctx);
            disk_access_cleanup();
            iso_write_cleanup();
            verification_cleanup();
            return 1;
        }
        
        // Audit verify start
        audit_action(AUDIT_ACTION_VERIFY_START, device_path, image_path, 0, false, "Starting verification");
        
        // Perform verification
        verification_progress_t progress;
        verify_result_t result = verify_iso_write(image_path, device_path, &progress);
        
        if (result == VERIFY_SUCCESS) {
            printf("\n=== VÉRIFICATION RÉUSSIE ===\n");
            printf("L'image ISO a été écrite correctement sur le périphérique.\n");
            printf("Le périphérique devrait être bootable.\n");
            
            // Audit verify success
            audit_action(AUDIT_ACTION_VERIFY_SUCCESS, device_path, image_path, iso_info.size_bytes, true, "Verification completed successfully");
        } else {
            printf("\n=== VÉRIFICATION ÉCHOUÉE ===\n");
            
            // Audit verify failure
            audit_action(AUDIT_ACTION_VERIFY_FAILED, device_path, image_path, 0, false, "Verification failed");
            
            switch (result) {
                case VERIFY_ERROR_INVALID_ISO:
                    printf("Fichier ISO invalide\n");
                    break;
                case VERIFY_ERROR_INVALID_DEVICE:
                    printf("Périphérique invalide\n");
                    break;
                case VERIFY_ERROR_HASH_MISMATCH:
                    printf("Les hashes ne correspondent pas! L'écriture a échoué.\n");
                    printf("Veuillez réessayer l'écriture.\n");
                    break;
                case VERIFY_ERROR_CANCELLED:
                    printf("Vérification annulée par l'utilisateur\n");
                    break;
                default:
                    printf("Erreur lors de la vérification (code: %d)\n", result);
                    break;
            }
        }
    }
    
    // Security mode
    if (security_mode) {
        printf("=== Mode Sécurité Avancée ===\n");
        enable_security_monitoring();
        printf("Monitoring de sécurité activé\n");
        printf("Logs: ~/.rufus.log\n");
        printf("Audit: %s\n", get_audit_file());
    }
    
    // Show logs
    if (show_logs) {
        printf("=== Logs Récents ===\n");
        print_recent_logs(20);
    }
    
    // Show audit
    if (show_audit) {
        printf("=== Audit des Opérations ===\n");
        print_recent_audit(20);
    }
    
    // List filesystems
    if (list_filesystems) {
        printf("=== Systèmes de Fichiers Supportés ===\n");
        
        filesystem_info_t filesystems[FS_MAX];
        int count = 0;
        
        if (list_supported_filesystems(filessystems, &count, FS_MAX)) {
            for (int i = 0; i < count; i++) {
                printf("[%d] %s\n", i + 1, filesystems[i].name);
                printf("    Taille: %.1f MB - %.1f %s\n", 
                       (double)filesystems[i].min_size_bytes / (1024*1024),
                       filesystems[i].max_size_bytes >= (1024ULL*1024*1024*1024) ? 
                       (double)filesystems[i].max_size_bytes / (1024LL*1024*1024*1024) : 
                       (double)filesystems[i].max_size_bytes / (1024*1024*1024),
                       filesystems[i].max_size_bytes >= (1024ULL*1024*1024*1024) ? "TB" : "GB");
                printf("    Linux Natif: %s\n", filesystems[i].is_native_linux ? "Oui" : "Non");
                printf("    Journaling: %s\n", filesystems[i].supports_journaling ? "Oui" : "Non");
                printf("    Fichiers >4GB: %s\n", filesystems[i].supports_large_files ? "Oui" : "Non");
                printf("\n");
            }
        } else {
            printf("Aucun système de fichiers supporté trouvé\n");
            printf("Installez les paquets: dosfstools, ntfs-3g, udftools, e2fsprogs, xfsprogs, btrfs-progs, f2fs-tools\n");
        }
    }
    
    // Format device
    if (format_device) {
        if (!device_path) {
            fprintf(stderr, "Erreur: --format nécessite --device DEV\n");
            usb_cleanup(ctx);
            disk_access_cleanup();
            iso_write_cleanup();
            verification_cleanup();
            security_cleanup();
            filesystem_cleanup();
            return 1;
        }
        
        printf("=== Formatage de Périphérique ===\n");
        
        // Parse filesystem type
        filesystem_type_t fs_type = get_filesystem_type_from_string(filesystem_type);
        if (fs_type == FS_UNKNOWN) {
            fprintf(stderr, "Erreur: Type de système de fichiers invalide: %s\n", filesystem_type);
            usb_cleanup(ctx);
            disk_access_cleanup();
            iso_write_cleanup();
            verification_cleanup();
            security_cleanup();
            filesystem_cleanup();
            return 1;
        }
        
        // Get device info
        disk_info_t disk_info;
        if (!get_disk_info(device_path, &disk_info)) {
            fprintf(stderr, "Erreur: impossible d'obtenir les informations du périphérique\n");
            usb_cleanup(ctx);
            disk_access_cleanup();
            iso_write_cleanup();
            verification_cleanup();
            security_cleanup();
            filesystem_cleanup();
            return 1;
        }
        
        print_disk_info(&disk_info);
        
        // Validate filesystem for device
        if (!validate_filesystem_for_device(device_path, fs_type, disk_info.size_bytes)) {
            usb_cleanup(ctx);
            disk_access_cleanup();
            iso_write_cleanup();
            verification_cleanup();
            security_cleanup();
            filesystem_cleanup();
            return 1;
        }
        
        // Show filesystem info
        filesystem_info_t fs_info;
        if (get_filesystem_info(fs_type, &fs_info)) {
            print_filesystem_info(&fs_info);
        }
        
        // Validate label if provided
        if (filesystem_label && !validate_label(filesystem_label, fs_type)) {
            fprintf(stderr, "Erreur: Label invalide pour le système de fichiers %s\n", filesystem_type);
            usb_cleanup(ctx);
            disk_access_cleanup();
            iso_write_cleanup();
            verification_cleanup();
            security_cleanup();
            filesystem_cleanup();
            return 1;
        }
        
        // Get user confirmation
        if (!get_user_confirmation("Formater le périphérique", &disk_info)) {
            printf("Formatage annulé\n");
            usb_cleanup(ctx);
            disk_access_cleanup();
            iso_write_cleanup();
            verification_cleanup();
            security_cleanup();
            filesystem_cleanup();
            return 0;
        }
        
        // Format device
        printf("Formatage de %s avec %s...\n", device_path, filesystem_type);
        
        bool format_result = format_device(device_path, fs_type, filesystem_label, PARTITION_SCHEME_MBR, true);
        
        if (format_result) {
            printf("=== FORMATAGE TERMINÉ AVEC SUCCÈS ===\n");
            printf("Périphérique %s formaté avec succès en %s\n", device_path, filesystem_type);
            
            if (filesystem_label) {
                printf("Label: %s\n", filesystem_label);
            }
            
            // Check filesystem
            printf("Vérification du système de fichiers...\n");
            if (check_filesystem(device_path, fs_type)) {
                printf("Système de fichiers valide\n");
            } else {
                printf("AVERTISSEMENT: La vérification du système de fichiers a échoué\n");
            }
        } else {
            printf("=== FORMATAGE ÉCHOUÉ ===\n");
            fprintf(stderr, "Le formatage de %s a échoué\n", device_path);
        }
    }
    
    // Legacy check for backward compatibility
    if (device_path && image_path && !write_iso && !verify) {
        printf("AVERTISSEMENT: Utilisez --write-iso pour écrire une image ISO\n");
        printf("Périphérique cible: %s\n", device_path);
        printf("Image source: %s\n", image_path);
        printf("Commande recommandée: %s --write-iso --device %s --image %s\n", 
               argv[0], device_path, image_path);
    }
    
    // Cleanup
    usb_cleanup(ctx);
    disk_access_cleanup();
    iso_write_cleanup();
    verification_cleanup();
    security_cleanup();
    filesystem_cleanup();
    ui_cleanup();
    
    return 0;
}
