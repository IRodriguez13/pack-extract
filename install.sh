#!/usr/bin/env bash

set -e

echo "Installing pack and extract utilities..."

# Check if running with sudo privileges
if [[ $EUID -ne 0 ]]; then
   echo "This script must be run with sudo privileges"
   echo "Usage: sudo ./install.sh"
   exit 1
fi

# Check if scripts exist
if [[ ! -f "extract.sh" ]] || [[ ! -f "pack.sh" ]]; then
    echo "Error: extract.sh and pack.sh must be in the current directory"
    exit 1
fi

# Install to /usr/bin
echo "Installing extract to /usr/bin/extract..."
cp extract.sh /usr/bin/extract
chmod 755 /usr/bin/extract

echo "Installing pack to /usr/bin/pack..."
cp pack.sh /usr/bin/pack
chmod 755 /usr/bin/pack

echo "Installation completed successfully"
echo "You can now use 'extract' and 'pack' commands from anywhere"
