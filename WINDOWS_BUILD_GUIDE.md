# Rufus Linux - Windows Build Guide

## Overview

Since Rufus Linux is a Linux application, you need to use WSL (Windows Subsystem for Linux) to build it on Windows. This guide will walk you through the complete setup and build process.

## Prerequisites

### Windows Requirements
- Windows 10 version 2004 or higher (Build 19041 and above)
- OR Windows 11 (any version)

### Administrator Access
Some steps require administrator privileges for installing WSL and system packages.

---

## Step 1: Install WSL

### Method A: Automatic Installation (Recommended)
```cmd
# Open Command Prompt as Administrator and run:
wsl --install
```

This will:
- Enable WSL feature
- Download and install the latest Linux kernel
- Set WSL 2 as your default
- Install Ubuntu distribution

### Method B: Manual Installation
```cmd
# Enable WSL feature
dism.exe /online /enable-feature /featurename:Microsoft-Windows-Subsystem-Linux /all /norestart

# Enable Virtual Machine Platform
dism.exe /online /enable-feature /featurename:VirtualMachinePlatform /all /norestart

# Download and install WSL kernel
# Download from: https://aka.ms/wsl2kernel

# Set WSL 2 as default
wsl --set-default-version 2
```

### Restart Your Computer
After installation, restart your computer.

---

## Step 2: Install Ubuntu

### From Microsoft Store
1. Open Microsoft Store
2. Search for "Ubuntu"
3. Click "Get" or "Install"
4. Wait for installation to complete

### Initial Ubuntu Setup
1. Launch Ubuntu from Start Menu
2. Wait for installation to complete
3. Create your UNIX username and password
   - Username: lowercase letters only (e.g., "user")
   - Password: create a secure password
   - Confirm password

---

## Step 3: Setup Development Environment

### Option A: Use Setup Script (Recommended)
```cmd
# Navigate to Rufus Linux directory
cd C:\Users\bigk7\OneDrive\Desktop\rufus\rufus-linux

# Run setup script
scripts\setup-wsl.bat
```

### Option B: Manual Setup
```bash
# In Ubuntu terminal
sudo apt-get update
sudo apt-get install build-essential libusb-1.0-0-dev libblkid-dev libssl-dev pkg-config
```

---

## Step 4: Build Rufus Linux

### Method A: Use Build Script (Recommended)
```cmd
# From Windows Command Prompt
scripts\build-simple.bat
```

### Method B: Manual Build in WSL
```bash
# In Ubuntu terminal
cd /mnt/c/Users/bigk7/OneDrive/Desktop/rufus/rufus-linux

# Create directories
mkdir -p build/core build/cli bin

# Compile source files
gcc -Wall -Wextra -std=c11 -O2 -D_GNU_SOURCE -Iinclude \
    $(pkg-config --cflags libusb-1.0 blkid) \
    -c src/core/usb_detect.c -o build/core/usb_detect.o

gcc -Wall -Wextra -std=c11 -O2 -D_GNU_SOURCE -Iinclude \
    $(pkg-config --cflags libusb-1.0 blkid) \
    -c src/core/disk_access.c -o build/core/disk_access.o

gcc -Wall -Wextra -std=c11 -O2 -D_GNU_SOURCE -Iinclude \
    $(pkg-config --cflags libusb-1.0 blkid) \
    -c src/core/iso_write.c -o build/core/iso_write.o

gcc -Wall -Wextra -std=c11 -O2 -D_GNU_SOURCE -Iinclude \
    $(pkg-config --cflags libusb-1.0 blkid) \
    -c src/core/verification.c -o build/core/verification.o

gcc -Wall -Wextra -std=c11 -O2 -D_GNU_SOURCE -Iinclude \
    $(pkg-config --cflags libusb-1.0 blkid) \
    -c src/core/security.c -o build/core/security.o

gcc -Wall -Wextra -std=c11 -O2 -D_GNU_SOURCE -Iinclude \
    $(pkg-config --cflags libusb-1.0 blkid) \
    -c src/core/filesystem.c -o build/core/filesystem.o

gcc -Wall -Wextra -std=c11 -O2 -D_GNU_SOURCE -Iinclude \
    $(pkg-config --cflags libusb-1.0 blkid) \
    -c src/core/ui.c -o build/core/ui.o

gcc -Wall -Wextra -std=c11 -O2 -D_GNU_SOURCE -Iinclude \
    $(pkg-config --cflags libusb-1.0 blkid) \
    -c src/cli/main.c -o build/cli/main.o

# Link binary
gcc build/core/*.o build/cli/*.o -o bin/rufus-linux \
    $(pkg-config --libs libusb-1.0 blkid) -lpthread -lssl -lcrypto -lm

# Make executable
chmod +x bin/rufus-linux

# Test binary
./bin/rufus-linux --version
```

---

## Step 5: Create Debian Package

### Method A: Use Package Script (Recommended)
```cmd
# From Windows Command Prompt
scripts\create-deb-simple.bat
```

### Method B: Manual Package Creation
```bash
# In Ubuntu terminal
cd /mnt/c/Users/bigk7/OneDrive/Desktop/rufus/rufus-linux

# Create package structure
rm -rf rufus-linux-package
mkdir -p rufus-linux-package/DEBIAN
mkdir -p rufus-linux-package/usr/local/bin

# Copy binary
cp bin/rufus-linux rufus-linux-package/usr/local/bin/
chmod +x rufus-linux-package/usr/local/bin/rufus-linux

# Create control file
cat > rufus-linux-package/DEBIAN/control << 'EOF'
Package: rufus-linux
Version: 1.0.0
Section: utils
Priority: optional
Architecture: amd64
Depends: libc6, libusb-1.0-0, libblkid1, libssl1.1, libcrypto1.1, libpthread0
Maintainer: Rufus Linux Project <contact@rufus-linux.org>
Description: Rufus-like USB bootable tool for Linux
 Rufus Linux is a complete port of the Windows Rufus utility to Linux.
 It provides a comprehensive CLI and interactive interface for creating
 bootable USB devices from ISO images.
EOF

# Build package
dpkg-deb --build rufus-linux-package
```

---

## Step 6: Install and Test

### Install Package
```bash
# In Ubuntu terminal
sudo dpkg -i rufus-linux_1.0.0_amd64.deb

# Fix dependencies if needed
sudo apt-get install -f
```

### Test Installation
```bash
# Test the binary
rufus-linux --version
rufus-linux --help

# Test interactive mode
rufus-linux --interactive
```

---

## Troubleshooting

### Common Issues

#### "WSL not recognized"
```cmd
# Enable WSL feature
dism.exe /online /enable-feature /featurename:Microsoft-Windows-Subsystem-Linux /all /norestart
# Restart computer
```

#### "Ubuntu not found"
```cmd
# Install Ubuntu from Microsoft Store
# Or install via command line:
wsl --install -d Ubuntu
```

#### "gcc: command not found"
```bash
# Install build tools
sudo apt-get update
sudo apt-get install build-essential
```

#### "libusb-1.0 not found"
```bash
# Install development libraries
sudo apt-get install libusb-1.0-0-dev libblkid-dev libssl-dev pkg-config
```

#### "Permission denied accessing USB"
```bash
# Add user to plugdev group
sudo usermod -a -G plugdev $USER
# Log out and log back in
```

#### Build fails with linking errors
```bash
# Check if libraries are installed
pkg-config --exists libusb-1.0 && echo "libusb-1.0: OK"
pkg-config --exists blkid && echo "blkid: OK"
pkg-config --exists openssl && echo "openssl: OK"
```

### WSL Issues

#### Systemd user session failed
```bash
# This is usually harmless and can be ignored
# Or fix by editing /etc/wsl.conf
echo "[boot]" | sudo tee -a /etc/wsl.conf
echo "systemd=true" | sudo tee -a /etc/wsl.conf
# Restart WSL
```

#### File permission issues
```bash
# Fix permissions for Windows files
sudo chmod -R 755 /mnt/c/Users/bigk7/OneDrive/Desktop/rufus/rufus-linux
```

---

## Alternative: Use Linux VM

If WSL doesn't work properly, you can use a Linux VM:

### Using VirtualBox
1. Download and install VirtualBox
2. Download Ubuntu Server ISO
3. Create new VM (2GB RAM, 20GB disk)
4. Install Ubuntu
5. Install VirtualBox Guest Additions
6. Share the Rufus Linux folder
7. Follow Linux build instructions

### Using VMware
1. Download and install VMware Player (free)
2. Download Ubuntu Desktop ISO
3. Create new VM
4. Install Ubuntu
5. Install VMware Tools
6. Share the Rufus Linux folder
7. Follow Linux build instructions

---

## Quick Start Summary

For experienced users, here's the quick version:

```cmd
# 1. Install WSL
wsl --install

# 2. Restart computer

# 3. Launch Ubuntu from Start Menu
# Create username/password

# 4. Setup development environment
wsl -d Ubuntu -- bash -c "sudo apt-get update && sudo apt-get install -y build-essential libusb-1.0-0-dev libblkid-dev libssl-dev pkg-config"

# 5. Build
cd C:\Users\bigk7\OneDrive\Desktop\rufus\rufus-linux
scripts\build-simple.bat

# 6. Create package
scripts\create-deb-simple.bat

# 7. Install
wsl -d Ubuntu -- sudo dpkg -i rufus-linux_1.0.0_amd64.deb
```

---

## Final Notes

- The resulting `.deb` file can be copied to any Linux system for installation
- You can also copy the `bin/rufus-linux` binary directly to Linux systems
- For development, it's recommended to work directly in the WSL Ubuntu environment
- The Windows batch scripts are just convenience wrappers around WSL commands

Once built, Rufus Linux will work on any Linux system with the required dependencies installed.
