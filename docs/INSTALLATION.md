# Rufus Linux - Complete Installation Guide

## Overview

Rufus Linux is a complete, production-ready Linux utility for creating bootable USB drives. This guide covers all installation methods from basic compilation to Debian package installation.

## Quick Start

### Option 1: Debian Package (Recommended)

```bash
# Build the package
./scripts/build.sh
./scripts/create-deb.sh

# Install
sudo dpkg -i rufus-linux_1.0.0_amd64.deb

# Run
rufus-linux --help
```

### Option 2: Direct Installation

```bash
# Build
./scripts/build.sh

# Install
sudo ./scripts/install.sh

# Run
rufus-linux --interactive
```

## Detailed Installation

### Prerequisites

#### Required Libraries
```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install build-essential libusb-1.0-0-dev libblkid-dev libssl-dev pkg-config

# RHEL/CentOS/Fedora
sudo yum groupinstall "Development Tools"
sudo yum install libusb-devel libblkid-devel openssl-devel pkgconfig

# Arch Linux
sudo pacman -S base-devel libusb libblkid openssl pkgconf
```

#### Verify Dependencies
```bash
# Check GCC
gcc --version

# Check libraries
pkg-config --exists libusb-1.0 && echo "libusb-1.0: OK"
pkg-config --exists blkid && echo "blkid: OK"
pkg-config --exists openssl && echo "openssl: OK"
```

### Method 1: Build from Source

#### Step 1: Build the Binary
```bash
# Using build script (recommended)
./scripts/build.sh

# Or using Makefile
make -f Makefile.build

# Or manual compilation (see BUILD.md)
```

#### Step 2: Install
```bash
# System-wide installation
sudo ./scripts/install.sh

# User installation
./scripts/install.sh

# Manual installation
sudo cp bin/rufus-linux /usr/local/bin/
sudo chmod +x /usr/local/bin/rufus-linux
sudo ln -s /usr/local/bin/rufus-linux /usr/local/bin/rufus
```

#### Step 3: Verify Installation
```bash
rufus-linux --version
rufus-linux --help
```

### Method 2: Debian Package

#### Step 1: Build Package
```bash
# Build the binary first
./scripts/build.sh

# Create Debian package
./scripts/create-deb.sh
```

#### Step 2: Install Package
```bash
# Install the package
sudo dpkg -i rufus-linux_1.0.0_amd64.deb

# Fix dependencies if needed
sudo apt-get install -f
```

#### Step 3: Verify Installation
```bash
# Check installation
dpkg -l | grep rufus-linux

# Test the binary
rufus-linux --version
```

### Method 3: Manual Compilation

#### Step 1: Prepare Environment
```bash
# Create directories
mkdir -p build/core build/cli bin

# Set environment variables
export CFLAGS="-Wall -Wextra -std=c11 -O2 -D_GNU_SOURCE"
export LIBS="-lusb-1.0 -lblkid -lpthread -lssl -lcrypto -lm"
export INCLUDES="-Iinclude"
```

#### Step 2: Compile Sources
```bash
# Compile core modules
gcc $CFLAGS $INCLUDES -c src/core/usb_detect.c -o build/core/usb_detect.o
gcc $CFLAGS $INCLUDES -c src/core/disk_access.c -o build/core/disk_access.o
gcc $CFLAGS $INCLUDES -c src/core/iso_write.c -o build/core/iso_write.o
gcc $CFLAGS $INCLUDES -c src/core/verification.c -o build/core/verification.o
gcc $CFLAGS $INCLUDES -c src/core/security.c -o build/core/security.o
gcc $CFLAGS $INCLUDES -c src/core/filesystem.c -o build/core/filesystem.o
gcc $CFLAGS $INCLUDES -c src/core/ui.c -o build/core/ui.o

# Compile CLI
gcc $CFLAGS $INCLUDES -c src/cli/main.c -o build/cli/main.o
```

#### Step 3: Link Binary
```bash
# Link with libraries
gcc build/core/*.o build/cli/*.o -o bin/rufus-linux $LIBS

# Make executable
chmod +x bin/rufus-linux
```

#### Step 4: Install
```bash
# Copy to system path
sudo cp bin/rufus-linux /usr/local/bin/
sudo chmod +x /usr/local/bin/rufus-linux
```

## Post-Installation Setup

### USB Access Permissions

#### Method 1: Add User to plugdev Group
```bash
# Add current user to plugdev group
sudo usermod -a -G plugdev $USER

# Log out and log back in, or run:
newgrp plugdev
```

#### Method 2: Create udev Rule
```bash
# Create udev rule file
sudo tee /etc/udev/rules.d/99-rufus-linux.rules > /dev/null << 'EOF'
# Rufus Linux USB access rule
KERNEL=="sd[a-z][0-9]*", SUBSYSTEM=="block", GROUP="plugdev", MODE="0666"
EOF

# Reload udev rules
sudo udevadm control --reload-rules
sudo udevadm trigger
```

### Verify USB Access
```bash
# Test USB access
ls -la /dev/sd*

# Run Rufus Linux test
rufus-linux --list-usb
```

## Usage Examples

### Basic Operations
```bash
# List USB devices
rufus-linux --list-usb

# List block devices
rufus-linux --list-disks

# Check device safety
rufus-linux --check-device /dev/sdb
```

### Creating Bootable USB
```bash
# Write ISO to USB
rufus-linux --write-iso --device /dev/sdb --image ubuntu.iso

# Verify write
rufus-linux --verify --device /dev/sdb --image ubuntu.iso

# Format USB
rufus-linux --format fat32 --device /dev/sdb --label USBSTICK
```

### Interactive Mode
```bash
# Launch interactive interface
rufus-linux --interactive

# Or short form
rufus-linux -I
```

### Security Mode
```bash
# Enable advanced security
rufus-linux --security --write-iso --device /dev/sdb --image ubuntu.iso

# View logs
rufus-linux --logs

# View audit
rufus-linux --audit
```

## Troubleshooting

### Common Issues

#### "Permission Denied" Accessing USB Devices
```bash
# Solution 1: Add user to plugdev group
sudo usermod -a -G plugdev $USER
newgrp plugdev

# Solution 2: Run as root (not recommended)
sudo rufus-linux --list-usb
```

#### "libusb-1.0 not found"
```bash
# Ubuntu/Debian
sudo apt-get install libusb-1.0-0-dev

# RHEL/CentOS
sudo yum install libusb-devel

# Arch Linux
sudo pacman -S libusb
```

#### "Device Not Found"
```bash
# Check if device exists
ls -la /dev/sd*

# Check if device is mounted
mount | grep /dev/sd

# Unmount if necessary
sudo umount /dev/sdb*
```

#### Build Errors
```bash
# Clean and rebuild
make -f Makefile.build clean
make -f Makefile.build

# Check dependencies
make -f Makefile.build check-deps
```

### Getting Help

#### Built-in Help
```bash
rufus-linux --help
rufus-linux --version
```

#### Debug Mode
```bash
# Enable debug logging
rufus-linux --security --logs

# Check system information
uname -a
lsb_release -a
```

## Uninstallation

### Remove Binary Installation
```bash
# Using uninstall script
sudo ./scripts/uninstall.sh

# Or manually
sudo rm -f /usr/local/bin/rufus-linux
sudo rm -f /usr/local/bin/rufus
```

### Remove Debian Package
```bash
# Remove package
sudo dpkg -r rufus-linux

# Remove configuration files
sudo dpkg -P rufus-linux
```

### Clean Build Files
```bash
# Using Makefile
make -f Makefile.build clean

# Or manually
rm -rf build bin
```

## System Requirements

### Minimum Requirements
- **OS**: Linux (Ubuntu 18.04+, RHEL 7+, Arch Linux)
- **Architecture**: x86_64 (amd64)
- **RAM**: 512MB minimum
- **Storage**: 100MB for binary and dependencies
- **USB**: USB 2.0+ port and device

### Recommended Requirements
- **OS**: Ubuntu 20.04+, RHEL 8+, Arch Linux
- **RAM**: 1GB+
- **Storage**: 1GB+ for ISO files
- **USB**: USB 3.0+ for better performance

## Security Considerations

### Data Protection
- **Always verify device paths** before writing
- **Use --check-device** to validate safety
- **Backup important data** before operations
- **Read confirmations carefully** - type "oui" explicitly

### System Safety
- **Never target /dev/sda** (system disk)
- **Use interactive mode** for better control
- **Enable security mode** for enhanced protection
- **Review logs** for operation history

## Support

### Documentation
- **README.md**: Project overview and quick start
- **BUILD.md**: Detailed build instructions
- **man page**: `man rufus-linux` (if installed)

### Community
- **GitHub**: https://github.com/rufus-linux/rufus-linux
- **Issues**: Report bugs and feature requests
- **Wiki**: Additional documentation and guides

## License

Rufus Linux is released under GPL v3, compatible with the original Rufus project.

---

**WARNING**: This software can permanently destroy data. Use with extreme caution and always verify device paths before operations.
