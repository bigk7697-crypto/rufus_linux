#!/bin/bash

# Rufus Linux Install Script
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
BIN_DIR="$PROJECT_ROOT/bin"
BINARY_NAME="rufus-linux"
BINARY_PATH="$BIN_DIR/$BINARY_NAME"

# Installation directories
INSTALL_DIR="/usr/local/bin"
MAN_DIR="/usr/local/share/man/man1"

echo -e "${BLUE}Rufus Linux Install Script${NC}"
echo -e "${BLUE}===========================${NC}"

# Check if running as root for system-wide installation
if [ "$EUID" -eq 0 ]; then
    echo -e "${YELLOW}Running as root - installing system-wide${NC}"
    INSTALL_DIR="/usr/local/bin"
    MAN_DIR="/usr/local/share/man/man1"
else
    echo -e "${YELLOW}Running as user - installing to $HOME/.local/bin${NC}"
    INSTALL_DIR="$HOME/.local/bin"
    MAN_DIR="$HOME/.local/share/man/man1"
    
    # Create user directories if they don't exist
    mkdir -p "$INSTALL_DIR"
    mkdir -p "$(dirname "$MAN_DIR")"
fi

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

# Install binary
echo -e "${YELLOW}Installing binary to $INSTALL_DIR...${NC}"
cp "$BINARY_PATH" "$INSTALL_DIR/$BINARY_NAME"

# Create symlink for convenience
echo -e "${YELLOW}Creating symlink...${NC}"
ln -sf "$INSTALL_DIR/$BINARY_NAME" "$INSTALL_DIR/rufus"

# Test installation
echo -e "${YELLOW}Testing installation...${NC}"
if "$INSTALL_DIR/$BINARY_NAME" --version &> /dev/null; then
    echo -e "${GREEN}Installation test passed!${NC}"
else
    echo -e "${RED}Error: Installation test failed${NC}"
    exit 1
fi

# Set up PATH if not in system PATH
if [ "$EUID" -ne 0 ]; then
    if ! echo "$PATH" | grep -q "$HOME/.local/bin"; then
        echo -e "${YELLOW}Adding $HOME/.local/bin to PATH...${NC}"
        echo 'export PATH="$HOME/.local/bin:$PATH"' >> "$HOME/.bashrc"
        echo -e "${YELLOW}Please run: source ~/.bashrc${NC}"
        echo -e "${YELLOW}Or log out and log back in${NC}"
    fi
fi

# Show installation info
echo -e "${GREEN}Installation successful!${NC}"
echo -e "${BLUE}Installation Information:${NC}"
echo "  Binary: $INSTALL_DIR/$BINARY_NAME"
echo "  Symlink: $INSTALL_DIR/rufus"
echo "  User: $(whoami)"

# Show usage
echo -e "${BLUE}Usage:${NC}"
echo "  $BINARY_NAME --help"
echo "  rufus --help"
echo "  $BINARY_NAME --interactive"

if [ "$EUID" -eq 0 ]; then
    echo -e "${BLUE}System-wide installation complete!${NC}"
else
    echo -e "${BLUE}User installation complete!${NC}"
    echo -e "${YELLOW}Note: Make sure $HOME/.local/bin is in your PATH${NC}"
fi

echo -e "${GREEN}Installation completed successfully!${NC}"
