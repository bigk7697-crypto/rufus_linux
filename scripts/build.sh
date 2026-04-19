#!/bin/bash

# Rufus Linux Build Script
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
BUILD_DIR="$PROJECT_ROOT/build"
BIN_DIR="$PROJECT_ROOT/bin"
SRC_DIR="$PROJECT_ROOT/src"
INCLUDE_DIR="$PROJECT_ROOT/include"

# Binary name
BINARY_NAME="rufus-linux"
BINARY_PATH="$BIN_DIR/$BINARY_NAME"

echo -e "${BLUE}Rufus Linux Build Script${NC}"
echo -e "${BLUE}=========================${NC}"

# Check dependencies
echo -e "${YELLOW}Checking dependencies...${NC}"

# Check for GCC
if ! command -v gcc &> /dev/null; then
    echo -e "${RED}Error: GCC is not installed${NC}"
    echo "Please install build-essential:"
    echo "  Ubuntu/Debian: sudo apt-get install build-essential"
    echo "  RHEL/CentOS: sudo yum groupinstall 'Development Tools'"
    echo "  Arch Linux: sudo pacman -S base-devel"
    exit 1
fi

# Check for required libraries
check_library() {
    local lib=$1
    local pkg_name=$2
    
    if ! pkg-config --exists "$lib" 2>/dev/null; then
        echo -e "${RED}Error: $lib development library not found${NC}"
        echo "Please install $pkg_name:"
        case "$(lsb_release -si 2>/dev/null || echo 'unknown')" in
            Ubuntu|Debian)
                echo "  sudo apt-get install $pkg_name"
                ;;
            RHEL*|CentOS*|Fedora)
                echo "  sudo yum install $pkg_name"
                ;;
            Arch*)
                echo "  sudo pacman -S $pkg_name"
                ;;
            *)
                echo "  Install $pkg_name for your distribution"
                ;;
        esac
        exit 1
    fi
}

check_library "libusb-1.0" "libusb-1.0-0-dev"
check_library "blkid" "libblkid-dev"
check_library "openssl" "libssl-dev"

echo -e "${GREEN}All dependencies found!${NC}"

# Create directories
echo -e "${YELLOW}Creating directories...${NC}"
mkdir -p "$BUILD_DIR"
mkdir -p "$BIN_DIR"

# Clean previous build
echo -e "${YELLOW}Cleaning previous build...${NC}"
rm -f "$BINARY_PATH"
rm -f "$BUILD_DIR"/*.o

# Get source files
echo -e "${YELLOW}Finding source files...${NC}"
SOURCES=$(find "$SRC_DIR" -name "*.c" -type f)
HEADERS=$(find "$INCLUDE_DIR" -name "*.h" -type f)

if [ -z "$SOURCES" ]; then
    echo -e "${RED}Error: No source files found${NC}"
    exit 1
fi

echo "Found $(echo "$SOURCES" | wc -l) source files:"
echo "$SOURCES" | sed 's|.*/||' | sed 's/^/  /'

# Compile flags
CFLAGS="-Wall -Wextra -std=c11 -O2 -D_GNU_SOURCE"
CFLAGS="$CFLAGS -I$INCLUDE_DIR"

# Link flags
LIBS="-lusb-1.0 -lblkid -lpthread -lssl -lcrypto -lm"

# Get library flags from pkg-config
if pkg-config --exists libusb-1.0; then
    CFLAGS="$CFLAGS $(pkg-config --cflags libusb-1.0)"
    LIBS="$LIBS $(pkg-config --libs libusb-1.0)"
fi

if pkg-config --exists blkid; then
    CFLAGS="$CFLAGS $(pkg-config --cflags blkid)"
    LIBS="$LIBS $(pkg-config --libs blkid)"
fi

echo -e "${YELLOW}Compiling...${NC}"

# Compile each source file
OBJECT_FILES=""
for source in $SOURCES; do
    obj_file="$BUILD_DIR/$(basename "$source" .c).o"
    echo "  Compiling $(basename "$source")..."
    
    if ! gcc $CFLAGS -c "$source" -o "$obj_file"; then
        echo -e "${RED}Error: Failed to compile $source${NC}"
        exit 1
    fi
    
    OBJECT_FILES="$OBJECT_FILES $obj_file"
done

# Link
echo -e "${YELLOW}Linking...${NC}"
echo "  Linking $BINARY_NAME..."

if ! gcc $OBJECT_FILES -o "$BINARY_PATH" $LIBS; then
    echo -e "${RED}Error: Failed to link $BINARY_NAME${NC}"
    exit 1
fi

# Make executable
chmod +x "$BINARY_PATH"

# Check if binary works
echo -e "${YELLOW}Testing binary...${NC}"
if "$BINARY_PATH" --version &> /dev/null; then
    echo -e "${GREEN}Binary test passed!${NC}"
else
    echo -e "${RED}Error: Binary test failed${NC}"
    exit 1
fi

# Show build info
echo -e "${GREEN}Build successful!${NC}"
echo -e "${BLUE}Build Information:${NC}"
echo "  Binary: $BINARY_PATH"
echo "  Size: $(du -h "$BINARY_PATH" | cut -f1)"
echo "  Type: $(file "$BINARY_PATH" | cut -d: -f2- | xargs)"

# Show usage
echo -e "${BLUE}Usage:${NC}"
echo "  $BINARY_PATH --help"
echo "  $BINARY_PATH --interactive"
echo "  $BINARY_PATH --list-usb"

echo -e "${GREEN}Build completed successfully!${NC}"
