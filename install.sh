#!/usr/bin/env bash

set -e

echo "Installing pack and extract utilities..."

if [[ ! -f "extract.sh" ]] || [[ ! -f "pack.sh" ]]; then
    echo "Error: extract.sh and pack.sh must be in the current directory"
    exit 1
fi

if [[ $EUID -ne 0 ]]; then

	mkdir -p "$HOME/.local/bin"
	mkdir -p "$HOME/.local/share/man/man1"
	mkdir -p "$HOME/.local/share/bash-completion/completions"
	mkdir -p "$HOME/.local/share/zsh/site-functions"

	echo "Installing extract to $HOME/.local/bin/extract..."
	cp extract.sh "$HOME/.local/bin/extract"
	chmod 755 "$HOME/.local/bin/extract"

	echo "Installing pack to $HOME/.local/bin/pack..."
	cp pack.sh "$HOME/.local/bin/pack"
	chmod 755 "$HOME/.local/bin/pack"

	if [[ -d "man" ]]; then
		echo "Installing man pages to $HOME/.local/share/man/man1/..."
		cp man/extract.1 "$HOME/.local/share/man/man1/extract.1"
		cp man/pack.1 "$HOME/.local/share/man/man1/pack.1"
	fi

	if [[ -d "completions" ]]; then
		echo "Installing bash completions..."
		cp completions/bash/extract "$HOME/.local/share/bash-completion/completions/extract"
		cp completions/bash/pack "$HOME/.local/share/bash-completion/completions/pack"

		echo "Installing zsh completions..."
		cp completions/zsh/_extract "$HOME/.local/share/zsh/site-functions/_extract"
		cp completions/zsh/_pack "$HOME/.local/share/zsh/site-functions/_pack"
	fi

	echo "Installation completed successfully"
	echo "You can now use 'extract' and 'pack' commands from anywhere"
fi

if [[ $EUID -eq 0 ]]; then

	echo "Installing extract to /usr/bin/extract..."
	cp extract.sh /usr/bin/extract
	chmod 755 /usr/bin/extract

	echo "Installing pack to /usr/bin/pack..."
	cp pack.sh /usr/bin/pack
	chmod 755 /usr/bin/pack

	if [[ -d "man" ]]; then
		echo "Installing man pages to /usr/share/man/man1/..."
		cp man/extract.1 /usr/share/man/man1/extract.1
		cp man/pack.1 /usr/share/man/man1/pack.1
	fi

	if [[ -d "completions" ]]; then
		echo "Installing bash completions..."
		cp completions/bash/extract /usr/share/bash-completion/completions/extract
		cp completions/bash/pack /usr/share/bash-completion/completions/pack

		if [[ -d "/usr/share/zsh/site-functions" ]]; then
			echo "Installing zsh completions..."
			cp completions/zsh/_extract /usr/share/zsh/site-functions/_extract
			cp completions/zsh/_pack /usr/share/zsh/site-functions/_pack
		fi
	fi

	echo "Installation completed successfully"
	echo "You can now use 'extract' and 'pack' commands from anywhere"
fi
