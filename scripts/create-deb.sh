#!/bin/bash

# Rufus Linux Debian Package Creator
# Copyright © 2026 Port Project

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Project directories
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
PACKAGE_DIR="$PROJECT_ROOT/rufus-linux-package"
BINARY_PATH="$PROJECT_ROOT/bin/rufus-linux"

# Package information
PACKAGE_NAME="rufus-linux"
PACKAGE_VERSION="1.0.0"
PACKAGE_ARCH="amd64"
DEB_FILE="$PACKAGE_NAME"_"$PACKAGE_VERSION"_"$PACKAGE_ARCH.deb"

echo -e "${BLUE}Rufus Linux Debian Package Creator${NC}"
echo -e "${BLUE}===================================${NC}"

# Check if binary exists
if [ ! -f "$BINARY_PATH" ]; then
    echo -e "${RED}Error: Binary not found at $BINARY_PATH${NC}"
    echo "Please build first: ./scripts/build.sh"
    exit 1
fi

# Check if binary is executable
if [ ! -x "$BINARY_PATH" ]; then
    echo -e "${YELLOW}Making binary executable...${NC}"
    chmod +x "$BINARY_PATH"
fi

# Create package directory structure
echo -e "${YELLOW}Creating package structure...${NC}"
rm -rf "$PACKAGE_DIR"
mkdir -p "$PACKAGE_DIR/DEBIAN"
mkdir -p "$PACKAGE_DIR/usr/local/bin"
mkdir -p "$PACKAGE_DIR/usr/local/share/man/man1"

# Copy binary
echo -e "${YELLOW}Copying binary...${NC}"
cp "$BINARY_PATH" "$PACKAGE_DIR/usr/local/bin/rufus-linux"
chmod +x "$PACKAGE_DIR/usr/local/bin/rufus-linux"

# Create symlink
echo -e "${YELLOW}Creating symlink...${NC}"
ln -s "rufus-linux" "$PACKAGE_DIR/usr/local/bin/rufus"

# Copy control file
echo -e "${YELLOW}Creating control file...${NC}"
cat > "$PACKAGE_DIR/DEBIAN/control" << EOF
Package: $PACKAGE_NAME
Version: $PACKAGE_VERSION
Section: utils
Priority: optional
Architecture: $PACKAGE_ARCH
Depends: libc6, libusb-1.0-0, libblkid1, libssl1.1, libcrypto1.1, libpthread0
Maintainer: Rufus Linux Project <contact@rufus-linux.org>
Description: Rufus-like USB bootable tool for Linux
 Rufus Linux is a complete port of the Windows Rufus utility to Linux.
 It provides a comprehensive CLI and interactive interface for creating
 bootable USB devices from ISO images.
 .
 Key features:
 - USB device detection with VID/PID information
 - High-performance ISO writing with progress tracking
 - Cryptographic verification (MD5, SHA1, SHA256)
 - Multi-filesystem support (FAT32, exFAT, NTFS, ext4, etc.)
 - Advanced security with logging and auditing
 - Interactive user interface with menus
 - MBR backup and restore functionality
 - System disk protection and safety checks
 .
 This package includes the rufus-linux binary and all necessary
 dependencies for creating bootable USB drives on Linux systems.
Homepage: https://github.com/rufus-linux/rufus-linux
EOF

# Create postinst script
echo -e "${YELLOW}Creating postinst script...${NC}"
cat > "$PACKAGE_DIR/DEBIAN/postinst" << 'EOF'
#!/bin/bash
set -e

# Add user to plugdev group for USB access
if getent group plugdev >/dev/null 2>&1; then
    if ! groups "$USER" | grep -q "\bplugdev\b"; then
        echo "Adding user $USER to plugdev group for USB access..."
        usermod -a -G plugdev "$USER"
        echo "You may need to log out and log back in for group changes to take effect."
    fi
fi

# Create symlink if it doesn't exist
if [ ! -L "/usr/local/bin/rufus" ]; then
    ln -s "/usr/local/bin/rufus-linux" "/usr/local/bin/rufus"
fi

echo "Installation complete!"
echo "Run 'rufus-linux --help' to get started."
EOF

# Create prerm script
echo -e "${YELLOW}Creating prerm script...${NC}"
cat > "$PACKAGE_DIR/DEBIAN/prerm" << 'EOF'
#!/bin/bash
set -e

# Remove symlink
if [ -L "/usr/local/bin/rufus" ]; then
    rm -f "/usr/local/bin/rufus"
fi
EOF

# Make scripts executable
chmod 755 "$PACKAGE_DIR/DEBIAN/postinst"
chmod 755 "$PACKAGE_DIR/DEBIAN/prerm"

# Set permissions
echo -e "${YELLOW}Setting permissions...${NC}"
chmod 644 "$PACKAGE_DIR/DEBIAN/control"
chmod 755 "$PACKAGE_DIR/usr/local/bin/rufus-linux"

# Calculate installed size
INSTALLED_SIZE=$(du -s "$PACKAGE_DIR/usr" | cut -f1)

# Update control file with installed size
sed -i "s/^Depends: .*/&\\nInstalled-Size: $INSTALLED_SIZE/" "$PACKAGE_DIR/DEBIAN/control"

# Build the package
echo -e "${YELLOW}Building Debian package...${NC}"
cd "$PROJECT_ROOT"
dpkg-deb --build "$PACKAGE_DIR"

# Check if package was created
if [ -f "$DEB_FILE" ]; then
    echo -e "${GREEN}Package created successfully!${NC}"
    echo -e "${BLUE}Package Information:${NC}"
    echo "  File: $DEB_FILE"
    echo "  Size: $(du -h "$DEB_FILE" | cut -f1)"
    
    # Show package info
    echo -e "${BLUE}Package Contents:${NC}"
    dpkg --info "$DEB_FILE" | head -20
    
    echo -e "${BLUE}Installation Instructions:${NC}"
    echo "  Install: sudo dpkg -i $DEB_FILE"
    echo "  Remove: sudo dpkg -r $PACKAGE_NAME"
    echo "  Run: rufus-linux --help"
    
    echo -e "${GREEN}Debian package creation completed successfully!${NC}"
else
    echo -e "${RED}Error: Package creation failed${NC}"
    exit 1
fi
