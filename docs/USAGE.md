# Rufus Linux - Usage Guide

## Overview

Rufus Linux provides both a comprehensive CLI interface and an interactive menu system for creating bootable USB drives. This guide covers all usage scenarios from basic operations to advanced features.

## Quick Reference

### Basic Commands
```bash
rufus-linux --help                    # Show help
rufus-linux --version                 # Show version
rufus-linux --list-usb                 # List USB devices
rufus-linux --list-disks               # List block devices
rufus-linux --interactive              # Interactive mode
```

### Core Operations
```bash
rufus-linux --write-iso --device /dev/sdb --image ubuntu.iso
rufus-linux --verify --device /dev/sdb --image ubuntu.iso
rufus-linux --format fat32 --device /dev/sdb --label USBSTICK
```

### Security Features
```bash
rufus-linux --security --write-iso --device /dev/sdb --image ubuntu.iso
rufus-linux --logs                     # Show recent logs
rufus-linux --audit                    # Show audit trail
```

## CLI Mode Usage

### Device Discovery

#### List USB Devices
```bash
$ rufus-linux --list-usb
=== Périphériques USB ===
[1] Bus 001 Device 002: ID 0781:5583 SanDisk Corp.
    Device: /dev/sdb
    Product: Ultra Fit
    Manufacturer: SanDisk
    Serial: 4C5300010710231178
    Storage Device: Yes
```

#### List All Block Devices
```bash
$ rufus-linux --list-disks
=== Périphériques de Bloc ===
[1] /dev/sda
    Size: 500.1 GB
    Model: Samsung SSD 860
    Removable: No
    System Disk: Yes
    
[2] /dev/sdb
    Size: 31.9 GB
    Model: SanDisk Ultra Fit
    Removable: Yes
    System Disk: No
```

#### List Storage Devices Only
```bash
$ rufus-linux --list-storage
=== Périphériques de Stockage USB ===
[1] Bus 001 Device 002: ID 0781:5583 SanDisk Corp.
    Device: /dev/sdb
    Product: Ultra Fit
    Storage Device: Yes
```

### Device Safety Checks

#### Check Device Safety
```bash
$ rufus-linux --check-device /dev/sdb
=== Vérification de Sécurité ===
Device: /dev/sdb
Size: 31.9 GB
Removable: Yes
System Disk: No
Mounted: No

=== RÉSULTAT DE SÉCURITÉ ===
Périphérique sécurisé pour l'écriture
```

#### Backup MBR
```bash
$ rufus-linux --backup-mbr /dev/sdb
=== Backup MBR ===
Device: /dev/sdb
Backup: mbr_backup_sdb_20260419_173027.bin
Size: 512 bytes
Status: Success
```

### ISO Operations

#### Write ISO to USB
```bash
$ rufus-linux --write-iso --device /dev/sdb --image ubuntu-22.04.iso
=== Écriture d'Image ISO ===
Image: ubuntu-22.04.iso
Size: 4.2 GB
Device: /dev/sdb
Size: 31.9 GB

=== Informations ISO ===
Type: ISO 9660
Bootable: Yes
Volume ID: Ubuntu 22.04.3 LTS

=== Informations Périphérique ===
Device: /dev/sdb
Size: 31.9 GB
Removable: Yes
System Disk: No

=== AVERTISSEMENTS DE SÉCURITÉ ===
Toutes les données sur /dev/sdb seront PERDUES!
Voulez-vous continuer? (tapez 'oui'): oui

=== ÉCRITURE EN COURS ===
[##################################################] 85.2% Écriture (reste: 1m 30s) (12.5 MB/s)

=== ÉCRITURE TERMINÉE AVEC SUCCÈS ===
Backup MBR disponible: mbr_backup_sdb_20260419_173027.bin
Périphérique prêt à être bootable
```

#### Verify ISO Write
```bash
$ rufus-linux --verify --device /dev/sdb --image ubuntu-22.04.iso
=== Vérification de l'Écriture ===
Image: ubuntu-22.04.iso
Device: /dev/sdb

=== Calcul des Hashes ===
Calcul des hashes de l'ISO...
  MD5: d41d8cd98f00b204e9800998ecf8427e
  SHA1: da39a3ee5e6b4b0d3255bfef95601890afd80709
  SHA256: e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855

Calcul des hashes du périphérique...
  MD5: d41d8cd98f00b204e9800998ecf8427e
  SHA1: da39a3ee5e6b4b0d3255bfef95601890afd80709
  SHA256: e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855

=== Comparaison des Hashes ===
MD5: CORRESPOND
SHA1: CORRESPOND
SHA256: CORRESPOND

=== VÉRIFICATION RÉUSSIE ===
L'image ISO a été écrite correctement sur le périphérique.
Le périphérique devrait être bootable.
```

### Filesystem Operations

#### List Supported Filesystems
```bash
$ rufus-linux --list-fs
=== Systèmes de Fichiers Supportés ===
[1] FAT32
    Taille: 32.0 MB - 2.0 TB
    Linux Natif: Non
    Journaling: Non
    Fichiers >4GB: Non

[2] exFAT
    Taille: 512.0 KB - 128.0 TB
    Linux Natif: Non
    Journaling: Non
    Fichiers >4GB: Oui

[3] NTFS
    Taille: 1.0 MB - 256.0 PB
    Linux Natif: Non
    Journaling: Oui
    Fichiers >4GB: Oui

[4] ext4
    Taille: 16.0 MB - 1024.0 TB
    Linux Natif: Oui
    Journaling: Oui
    Fichiers >4GB: Oui
```

#### Format Device
```bash
$ rufus-linux --format fat32 --device /dev/sdb --label USBSTICK
=== Formatage de Périphérique ===
Device: /dev/sdb
Size: 31.9 GB
Filesystem: FAT32
Label: USBSTICK

=== Filesystem Information ===
Type: FAT32
Min Size: 32.0 MB
Max Size: 2.0 TB
Large Files: No
mkfs Command: mkfs.fat

=== AVERTISSEMENTS DE SÉCURITÉ ===
Toutes les données sur /dev/sdb seront PERDUES!
Voulez-vous continuer? (tapez 'oui'): oui

=== FORMATAGE EN COURS ===
Formater /dev/sdb avec fat32...

=== FORMATAGE TERMINÉ AVEC SUCCÈS ===
Périphérique /dev/sdb formaté avec succès en fat32
Label: USBSTICK
```

### Security Features

#### Enable Security Mode
```bash
$ rufus-linux --security --write-iso --device /dev/sdb --image ubuntu.iso
=== Mode Sécurité Avancée ===
Monitoring de sécurité activé
Logs: ~/.rufus.log
Audit: ~/.rufus_audit.log

=== Écriture d'Image ISO ===
[Enhanced security checks enabled]
```

#### View Logs
```bash
$ rufus-linux --logs
=== Logs Récents ===
[2026-04-19 17:30:15] [INFO] Security system initialized
[2026-04-19 17:30:16] [INFO] USB scan completed: 1 devices found
[2026-04-19 17:30:17] [WARNING] Device /dev/sdb has no mounted partitions
[2026-04-19 17:30:18] [INFO] ISO validation successful: ubuntu-22.04.iso
[2026-04-19 17:32:45] [INFO] Write operation completed successfully
```

#### View Audit Trail
```bash
$ rufus-linux --audit
=== Audit des Opérations ===
[2026-04-19 17:30:15] [USB_DETECT] action=USB_DETECT device=none iso=none bytes=0 success=true
[2026-04-19 17:32:45] [WRITE_SUCCESS] action=WRITE_SUCCESS device=/dev/sdb iso=ubuntu-22.04.iso bytes=4508876800 success=true
[2026-04-19 17:35:12] [VERIFY_SUCCESS] action=VERIFY_SUCCESS device=/dev/sdb iso=ubuntu-22.04.iso bytes=4508876800 success=true
```

## Interactive Mode

### Launch Interactive Mode
```bash
$ rufus-linux --interactive
# or
$ rufus-linux -I
```

### Main Menu Navigation
```
=== Rufus Linux - Menu Principal ===
===============================================
         Rufus Linux - Menu Principal
===============================================

 > 1. Détecter les périphériques USB (1)
    Scanner et lister les périphériques USB connectés
   2. Lister les périphériques de bloc (2)
    Afficher tous les périphériques de bloc disponibles
   3. Écrire une image ISO (3)
    Écrire une image ISO sur un périphérique USB
   4. Formater un périphérique (4)
    Formater un périphérique avec un système de fichiers
   5. Vérifier l'intégrité (5)
    Vérifier l'écriture d'une image ISO
   6. Sécurité et Logs (6)
    Afficher les logs et les options de sécurité
   9. Quitter (q)
    Quitter le programme

===============================================
Sélectionnez une option (1-9, ou 'q' pour quitter): 
```

### Device Selection Menu
```
=== Sélection d'un Périphérique ===
===============================================
         Sélection du Périphérique Cible
===============================================

[1] /dev/sdb
    Taille: 31.9 GB
    Modèle: SanDisk Ultra Fit
    Type: Amovible

[2] /dev/sda
    Taille: 500.1 GB
    Modèle: Samsung SSD 860
    Type: Fixe [SYSTÈME]

===============================================
Sélectionnez un périphérique (1-2): 1
```

### Filesystem Selection Menu
```
=== Sélection d'un Système de Fichiers ===
===============================================
         Sélection d'un Système de Fichiers
===============================================

[1] FAT32
    Taille: 32.0 MB - 2.0 TB
    Linux Natif: Non
    Journaling: Non
    Fichiers >4GB: Non

[2] exFAT
    Taille: 512.0 KB - 128.0 TB
    Linux Natif: Non
    Journaling: Non
    Fichiers >4GB: Oui

[3] ext4
    Taille: 16.0 MB - 1024.0 TB
    Linux Natif: Oui
    Journaling: Oui
    Fichiers >4GB: Oui

===============================================
Sélectionnez un système de fichiers (1-3): 3
```

### Confirmation Dialogs
```
=== Écrire l'Image ISO ===
===============================================
         Écrire l'Image ISO
===============================================

Voulez-vous écrire cette image ISO sur le périphérique sélectionné?

Détails:
Périphérique: /dev/sdb
Taille: 31.9 GB
Image: /home/user/ubuntu-22.04.iso

AVERTISSEMENT: Cette opération peut détruire des données de manière irréversible!

===============================================
Confirmer l'opération? (tapez 'oui' pour continuer): oui
```

## Advanced Usage

### Batch Operations
```bash
# Write multiple ISOs in sequence
for iso in ubuntu.iso debian.iso fedora.iso; do
    rufus-linux --write-iso --device /dev/sdb --image "$iso"
    rufus-linux --verify --device /dev/sdb --image "$iso"
done
```

### Scripted Operations
```bash
#!/bin/bash
# Automated USB creation script

DEVICE="/dev/sdb"
ISO_IMAGE="ubuntu-22.04.iso"

# Check device safety
if ! rufus-linux --check-device "$DEVICE" | grep -q "sécurisé"; then
    echo "Device not safe for writing"
    exit 1
fi

# Write ISO
echo "Writing ISO to $DEVICE..."
rufus-linux --write-iso --device "$DEVICE" --image "$ISO_IMAGE"

# Verify
echo "Verifying write..."
rufus-linux --verify --device "$DEVICE" --image "$ISO_IMAGE"

echo "USB creation complete!"
```

### Security Mode Automation
```bash
# Enable security monitoring
export RUFUS_SECURITY_MODE=1

# Run with enhanced security
rufus-linux --security --write-iso --device /dev/sdb --image ubuntu.iso

# Check logs
rufus-linux --logs | tail -20
```

## Filesystem Recommendations

### Choosing the Right Filesystem

#### FAT32
- **Best for**: Maximum compatibility
- **Pros**: Works on all systems, including old computers
- **Cons**: 4GB file size limit, 2TB volume limit
- **Use case**: Creating boot drives for older systems

#### exFAT
- **Best for**: Large files and cross-platform use
- **Pros**: Supports files >4GB, large volumes
- **Cons**: Less compatible with very old systems
- **Use case**: Modern systems with large ISO files

#### NTFS
- **Best for**: Windows-focused environments
- **Pros**: Windows native, journaling, compression
- **Cons**: Limited Linux support without drivers
- **Use case**: Windows-only environments

#### ext4
- **Best for**: Linux-native environments
- **Pros**: Linux native, excellent performance, journaling
- **Cons**: Windows compatibility limited
- **Use case**: Linux-to-Linux USB drives

## Performance Tips

### Optimize Write Speed
```bash
# Use exFAT for large ISOs
rufus-linux --format exfat --device /dev/sdb --label FASTUSB

# Enable security mode for monitoring
rufus-linux --security --write-iso --device /dev/sdb --image large.iso
```

### USB 3.0 Optimization
- Use USB 3.0 ports and drives for better performance
- Expected speeds: 10-50 MB/s for USB 3.0, 5-20 MB/s for USB 2.0
- Large ISOs (>4GB) benefit from exFAT filesystem

### System Optimization
```bash
# Close unnecessary applications
# Use dedicated USB controller if available
# Ensure sufficient RAM for buffering
```

## Troubleshooting

### Common Error Messages

#### "Permission Denied"
```bash
# Add user to plugdev group
sudo usermod -a -G plugdev $USER
newgrp plugdev

# Or run with sudo (not recommended)
sudo rufus-linux --list-usb
```

#### "Device Not Found"
```bash
# Check device existence
ls -la /dev/sd*

# Check if device is mounted
mount | grep /dev/sd

# Unmount if necessary
sudo umount /dev/sdb*
```

#### "Write Failed"
```bash
# Check disk space
df -h /dev/sdb

# Check device health
sudo fsck /dev/sdb

# Try different USB port or cable
```

#### "Verification Failed"
```bash
# Try writing again
rufus-linux --write-iso --device /dev/sdb --image ubuntu.iso

# Check ISO integrity
md5sum ubuntu.iso

# Use different USB drive
```

### Debug Mode
```bash
# Enable verbose logging
rufus-linux --security --logs

# Check system information
uname -a
lsusb
```

## Safety Reminders

### Before Writing
1. **Verify device path** - Double-check /dev/sdX
2. **Backup important data** - All data will be lost
3. **Use safety checks** - Run --check-device first
4. **Read confirmations** - Type "oui" explicitly

### During Operations
1. **Don't interrupt** - Let operations complete
2. **Monitor progress** - Watch for error messages
3. **Keep system stable** - Avoid heavy disk I/O
4. **Verify results** - Use --verify after writing

### After Operations
1. **Test bootability** - Boot from the USB drive
2. **Check logs** - Review --audit trail
3. **Backup MBR** - Keep backup files safe
4. **Clean up** - Properly eject USB drive

## Examples by Use Case

### System Administrator
```bash
# Create multiple boot drives
for device in /dev/sdb /dev/sdc /dev/sdd; do
    rufus-linux --check-device "$device"
    rufus-linux --write-iso --device "$device" --image ubuntu-server.iso
    rufus-linux --verify --device "$device" --image ubuntu-server.iso
done
```

### Developer
```bash
# Test different ISOs
rufus-linux --format ext4 --device /dev/sdb --label TESTUSB
rufus-linux --write-iso --device /dev/sdb --image custom-build.iso
rufus-linux --verify --device /dev/sdb --image custom-build.iso
```

### Home User
```bash
# Simple boot drive creation
rufus-linux --interactive

# Or one-liner
rufus-linux --write-iso --device /dev/sdb --image ubuntu-22.04.iso
```

This comprehensive usage guide covers all Rufus Linux functionality from basic operations to advanced scripting and troubleshooting.
