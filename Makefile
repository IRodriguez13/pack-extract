# Makefile for pack-extract project
# Builds and installs the C versions of pack and extract utilities

CC = gcc
CFLAGS = -O2 -Wall -Wextra -pedantic
LDFLAGS = -larchive

# Build directory
BUILD_DIR = build

# Binaries
PACK = $(BUILD_DIR)/pack
EXTRACT = $(BUILD_DIR)/extract

# Sources
PACK_SRC = pack.c
EXTRACT_SRC = extract.c

# Installation directories
PREFIX = /usr/local
ifeq ($(shell id -u), 0)
    # Root installation
    BIN_DIR = $(PREFIX)/bin
    MAN_DIR = $(PREFIX)/share/man/man1
    BASH_COMPLETION_DIR = $(PREFIX)/share/bash-completion/completions
    ZSH_COMPLETION_DIR = $(PREFIX)/share/zsh/site-functions
else
    # User installation
    BIN_DIR = $(HOME)/.local/bin
    MAN_DIR = $(HOME)/.local/share/man/man1
    BASH_COMPLETION_DIR = $(HOME)/.local/share/bash-completion/completions
    ZSH_COMPLETION_DIR = $(HOME)/.local/share/zsh/site-functions
endif

# Man pages
MAN_PAGES = man/extract.1 man/pack.1

# Completions
BASH_COMPLETIONS = completions/bash/extract completions/bash/pack
ZSH_COMPLETIONS = completions/zsh/_extract completions/zsh/_pack

.PHONY: all clean install uninstall help

$(BUILD_DIR):
	mkdir -p $@

all: $(PACK) $(EXTRACT)

$(PACK): $(PACK_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

$(EXTRACT): $(EXTRACT_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -rf $(BUILD_DIR)

install: all
	@echo "Installing to $(BIN_DIR)..."
	mkdir -p $(BIN_DIR)
	cp $(PACK) $(BIN_DIR)/
	cp $(EXTRACT) $(BIN_DIR)/
	@if [ -d man ]; then \
		echo "Installing man pages to $(MAN_DIR)..."; \
		mkdir -p $(MAN_DIR); \
		cp $(MAN_PAGES) $(MAN_DIR)/; \
	fi
	@if [ -d completions ]; then \
		echo "Installing bash completions to $(BASH_COMPLETION_DIR)..."; \
		mkdir -p $(BASH_COMPLETION_DIR); \
		cp $(BASH_COMPLETIONS) $(BASH_COMPLETION_DIR)/; \
		echo "Installing zsh completions to $(ZSH_COMPLETION_DIR)..."; \
		mkdir -p $(ZSH_COMPLETION_DIR); \
		cp $(ZSH_COMPLETIONS) $(ZSH_COMPLETION_DIR)/; \
	fi
	@echo "Installation completed successfully."
	@echo "You can now use '$(PACK)' and '$(EXTRACT)' commands."

uninstall:
	@echo "Uninstalling from $(BIN_DIR)..."
	rm -f $(BIN_DIR)/$(PACK)
	rm -f $(BIN_DIR)/$(EXTRACT)
	@echo "Uninstalling man pages from $(MAN_DIR)..."
	rm -f $(MAN_DIR)/extract.1
	rm -f $(MAN_DIR)/pack.1
	@echo "Uninstalling completions..."
	rm -f $(BASH_COMPLETION_DIR)/extract
	rm -f $(BASH_COMPLETION_DIR)/pack
	rm -f $(ZSH_COMPLETION_DIR)/_extract
	rm -f $(ZSH_COMPLETION_DIR)/_pack
	@echo "Uninstallation completed."

help:
	@echo "Available targets:"
	@echo "  all       - Build pack and extract binaries in $(BUILD_DIR)/ (default)"
	@echo "  clean     - Remove $(BUILD_DIR) directory"
	@echo "  install   - Install binaries, man pages, and completions"
	@echo "  uninstall - Uninstall binaries, man pages, and completions"
	@echo "  help      - Show this help message"
	@echo ""
	@echo "Installation directories:"
	@echo "  BIN_DIR: $(BIN_DIR)"
	@echo "  MAN_DIR: $(MAN_DIR)"
	@echo "  BASH_COMPLETION_DIR: $(BASH_COMPLETION_DIR)"
	@echo "  ZSH_COMPLETION_DIR: $(ZSH_COMPLETION_DIR)"