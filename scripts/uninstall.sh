#!/bin/bash

# Rufus Linux Uninstall Script
# Copyright © 2026 Port Project

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Binary name
BINARY_NAME="rufus-linux"
SYMLINK_NAME="rufus"

# Installation directories
SYSTEM_INSTALL_DIR="/usr/local/bin"
USER_INSTALL_DIR="$HOME/.local/bin"

echo -e "${BLUE}Rufus Linux Uninstall Script${NC}"
echo -e "${BLUE}=============================${NC}"

# Function to remove if exists
remove_if_exists() {
    local file=$1
    if [ -f "$file" ]; then
        echo -e "${YELLOW}Removing $file${NC}"
        rm -f "$file"
    else
        echo -e "${BLUE}$file not found${NC}"
    fi
}

# Function to remove symlink if exists
remove_symlink_if_exists() {
    local link=$1
    if [ -L "$link" ]; then
        echo -e "${YELLOW}Removing symlink $link${NC}"
        rm -f "$link"
    else
        echo -e "${BLUE}Symlink $link not found${NC}"
    fi
}

# Check if running as root
if [ "$EUID" -eq 0 ]; then
    echo -e "${YELLOW}Running as root - removing system-wide installation${NC}"
    INSTALL_DIR="$SYSTEM_INSTALL_DIR"
else
    echo -e "${YELLOW}Running as user - removing user installation${NC}"
    INSTALL_DIR="$USER_INSTALL_DIR"
fi

# Remove binary
remove_if_exists "$INSTALL_DIR/$BINARY_NAME"

# Remove symlink
remove_symlink_if_exists "$INSTALL_DIR/$SYMLINK_NAME"

# Remove man page if exists
remove_if_exists "/usr/local/share/man/man1/$BINARY_NAME.1"
remove_if_exists "$HOME/.local/share/man/man1/$BINARY_NAME.1"

# Remove from PATH in .bashrc (user installation only)
if [ "$EUID" -ne 0 ]; then
    if [ -f "$HOME/.bashrc" ]; then
        echo -e "${YELLOW}Cleaning up .bashrc...${NC}"
        # Remove the line that adds .local/bin to PATH
        sed -i '/\.local\/bin/d' "$HOME/.bashrc" 2>/dev/null || true
    fi
fi

# Show completion message
echo -e "${GREEN}Uninstallation completed!${NC}"
echo -e "${BLUE}Removed:${NC}"
echo "  Binary: $INSTALL_DIR/$BINARY_NAME"
echo "  Symlink: $INSTALL_DIR/$SYMLINK_NAME"

if [ "$EUID" -eq 0 ]; then
    echo -e "${BLUE}System-wide uninstallation complete!${NC}"
else
    echo -e "${BLUE}User uninstallation complete!${NC}"
    echo -e "${YELLOW}Note: You may need to restart your terminal or run: source ~/.bashrc${NC}"
fi

echo -e "${GREEN}Uninstallation completed successfully!${NC}"
