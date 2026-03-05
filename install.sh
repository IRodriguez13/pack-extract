#!/usr/bin/env bash

set -e

echo "Installing pack and extract utilities..."


# Check if scripts exist
if [[ ! -f "extract.sh" ]] || [[ ! -f "pack.sh" ]]; then
    echo "Error: extract.sh and pack.sh must be in the current directory"
    exit 1
fi

# if is not running with sudo privileges install it in the user
if [[ $EUID -ne 0 ]]; then

	if [[ ! -d "$HOME/.local/bin" ]]; then
		mkdir -p "$HOME/.local/bin/"
	fi

	echo "Installing extract to $HOME/.local/bin/extract..."
	cp extract.sh $HOME/.local/bin/extract
	chmod 755 $HOME/.local/bin/extract

	echo "Installing pack to $HOME/.local/bin/pack"
	cp pack.sh $HOME/.local/bin/pack
	chmod 755 $HOME/.local/bin/pack

	echo "Installation completed successfully"
	echo "You can now use 'extract' and 'pack' commands from anywhere"
fi

# if is running with sudo privileges install it globally
if [[ $EUID -eq 0 ]]; then

# Install to /usr/bin
	echo "Installing extract to /usr/bin/extract..."
	cp extract.sh /usr/bin/extract
	chmod 755 /usr/bin/extract

	echo "Installing pack to /usr/bin/pack..."
	cp pack.sh /usr/bin/pack
	chmod 755 /usr/bin/pack
	echo "Installation completed successfully"
	echo "You can now use 'extract' and 'pack' commands from anywhere"
fi
