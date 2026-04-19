# Build Instructions for Rufus Linux

## Linux Executable Format

**Important**: Linux executables do NOT use ".exe" extension. They are ELF binaries that run directly from the terminal.

## Prerequisites

### Required Development Libraries

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

### Verify Dependencies

```bash
# Check GCC
gcc --version

# Check libraries
pkg-config --exists libusb-1.0 && echo "libusb-1.0: OK"
pkg-config --exists blkid && echo "blkid: OK"
pkg-config --exists openssl && echo "openssl: OK"
```

## Build Methods

### Method 1: Using the Build Script (Recommended)

```bash
# Clone or extract the project
cd rufus-linux

# Run the build script
./scripts/build.sh

# The binary will be created in bin/rufus-linux
```

### Method 2: Using Makefile

```bash
# Clone or extract the project
cd rufus-linux

# Check dependencies
make -f Makefile.build check-deps

# Build
make -f Makefile.build

# The binary will be created in bin/rufus-linux
```

### Method 3: Manual Compilation

```bash
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

# Link
gcc build/core/*.o build/cli/*.o -o bin/rufus-linux \
    $(pkg-config --libs libusb-1.0 blkid) -lpthread -lssl -lcrypto -lm

# Make executable
chmod +x bin/rufus-linux
```

## Verify the Build

```bash
# Check file type
file bin/rufus-linux
# Should show: ELF 64-bit LSB executable

# Check if it runs
./bin/rufus-linux --version

# Test help
./bin/rufus-linux --help
```

## Installation

### System-wide Installation (requires root)

```bash
# Using install script
sudo ./scripts/install.sh

# Or manually
sudo cp bin/rufus-linux /usr/local/bin/
sudo chmod +x /usr/local/bin/rufus-linux
sudo ln -s /usr/local/bin/rufus-linux /usr/local/bin/rufus
```

### User Installation

```bash
# Using install script
./scripts/install.sh

# Or manually
mkdir -p ~/.local/bin
cp bin/rufus-linux ~/.local/bin/
chmod +x ~/.local/bin/rufus-linux
ln -s ~/.local/bin/rufus-linux ~/.local/bin/rufus

# Add to PATH (add to ~/.bashrc)
echo 'export PATH="$HOME/.local/bin:$PATH"' >> ~/.bashrc
source ~/.bashrc
```

## Running the Program

```bash
# From project directory
./bin/rufus-linux --help

# After installation
rufus-linux --help
rufus-linux --interactive
rufus-linux --list-usb
```

## Troubleshooting

### Common Build Errors

1. **"gcc: command not found"**
   ```bash
   sudo apt-get install build-essential  # Ubuntu/Debian
   sudo yum groupinstall "Development Tools"  # RHEL/CentOS
   ```

2. **"libusb-1.0 not found"**
   ```bash
   sudo apt-get install libusb-1.0-0-dev  # Ubuntu/Debian
   sudo yum install libusb-devel  # RHEL/CentOS
   ```

3. **"blkid.h: No such file or directory"**
   ```bash
   sudo apt-get install libblkid-dev  # Ubuntu/Debian
   sudo yum install libblkid-devel  # RHEL/CentOS
   ```

4. **"openssl/ssl.h: No such file or directory"**
   ```bash
   sudo apt-get install libssl-dev  # Ubuntu/Debian
   sudo yum install openssl-devel  # RHEL/CentOS
   ```

### Permission Issues

```bash
# If you get "Permission denied" when accessing USB devices
sudo usermod -a -G plugdev $USER
# Log out and log back in
```

### Library Issues

```bash
# Check if required libraries are installed
ldd bin/rufus-linux

# If you see "not found" for any library, install the corresponding -dev package
```

## Binary Information

The resulting binary is an ELF executable:
- **Format**: ELF 64-bit LSB executable
- **Architecture**: x86-64
- **Linked**: Dynamically linked
- **Size**: Typically 100-500KB
- **Dependencies**: libusb-1.0, libblkid, libssl, libcrypto, libpthread

## Clean Build

```bash
# Using Makefile
make -f Makefile.build clean

# Or manually
rm -rf build bin
```

## Debug Build

```bash
# Using Makefile
make -f Makefile.build debug

# Or manually with -g flag
gcc -g -Wall -Wextra -std=c11 -DDEBUG ...
```
