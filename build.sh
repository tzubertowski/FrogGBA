#!/bin/bash

# TempGBA4PSP Build Script
# This script compiles the TempGBA4PSP emulator for PSP

set -e  # Exit on any error

echo "=== TempGBA4PSP Build Script ==="
echo "Setting up PSP development environment..."

# Set PSP development environment variables
export PSPDEV=/usr/local/pspdev
export PATH=$PATH:$PSPDEV/bin
export PSPSDK=$PSPDEV/psp/sdk

# Verify toolchain is installed
echo "Checking PSP toolchain..."
if ! command -v psp-gcc &> /dev/null; then
    echo "ERROR: PSP toolchain not found. Make sure the Docker image was built correctly."
    exit 1
fi

echo "PSP GCC version:"
psp-gcc --version | head -1

echo "PSP SDK path: $PSPSDK"
echo "PSP DEV path: $PSPDEV"

# Navigate to source directory
cd source

echo "=== Building TempGBA4PSP ==="
echo "Cleaning previous build..."
make clean || true

echo "Starting compilation..."
make

# Check if build was successful
if [ -f "TempGBA.prx" ] && [ -f "EBOOT.PBP" ]; then
    echo "=== BUILD SUCCESSFUL ==="
    echo "Generated files:"
    ls -la TempGBA.prx EBOOT.PBP
    
    # Create output directory
    mkdir -p ../build
    cp TempGBA.prx ../build/
    cp EBOOT.PBP ../build/
    
    echo "Build artifacts copied to build/ directory"
    echo "You can now copy EBOOT.PBP to your PSP's PSP/GAME/TempGBA/ folder"
else
    echo "=== BUILD FAILED ==="
    echo "Expected output files not found"
    exit 1
fi