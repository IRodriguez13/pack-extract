# Makefile for pack-extract project
# Builds and installs the C versions of pack and extract utilities

VERSION := $(shell cat VERSION)
CC = gcc
CFLAGS = -O2 -Wall -Wextra -pedantic -Iinclude -DPACK_EXTRACT_VERSION=\"$(VERSION)\"
LDFLAGS = -larchive

BUILD_DIR = build
PACK = $(BUILD_DIR)/pack
EXTRACT = $(BUILD_DIR)/extract
PACK_SRC = pack.c
EXTRACT_SRC = extract.c

PREFIX = /usr/local
ifeq ($(shell id -u), 0)
    BIN_DIR = $(PREFIX)/bin
    MAN_DIR = $(PREFIX)/share/man/man1
    BASH_COMPLETION_DIR = $(PREFIX)/share/bash-completion/completions
    BASH_COMPLETION_LEGACY_DIR = /etc/bash_completion.d
    ZSH_COMPLETION_DIR = $(PREFIX)/share/zsh/site-functions
else
    BIN_DIR = $(HOME)/.local/bin
    MAN_DIR = $(HOME)/.local/share/man/man1
    BASH_COMPLETION_DIR = $(HOME)/.local/share/bash-completion/completions
    BASH_COMPLETION_LEGACY_DIR = $(HOME)/.local/share/bash-completion/completions
    ZSH_COMPLETION_DIR = $(HOME)/.local/share/zsh/site-functions
endif

MAN_PAGES = man/extract.1 man/pack.1
BASH_COMPLETIONS = completions/bash/extract completions/bash/pack
ZSH_COMPLETIONS = completions/zsh/_extract completions/zsh/_pack
DIST_NAME = pack-extract-$(VERSION)
DIST_DIR = dist/$(DIST_NAME)
DIST_TAR = dist/$(DIST_NAME).tar.gz
PACK_FILES = VERSION Makefile README.md pack.c extract.c include man completions pack.sh extract.sh install.sh .gitignore

.PHONY: all pack extract clean install uninstall help dist-pack dist-extract release

$(BUILD_DIR):
	mkdir -p $@

all: $(PACK) $(EXTRACT)

pack-bin: $(PACK)

extract-bin: $(EXTRACT)

$(PACK): $(PACK_SRC) include/version.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

$(EXTRACT): $(EXTRACT_SRC) include/version.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -rf $(BUILD_DIR)

install: all
	@echo "Installing to $(BIN_DIR)..."
	mkdir -p $(BIN_DIR)
	cp $(PACK) $(BIN_DIR)/pack
	cp $(EXTRACT) $(BIN_DIR)/extract
	@if [ -d man ]; then \
		echo "Installing man pages to $(MAN_DIR)..."; \
		mkdir -p $(MAN_DIR); \
		cp $(MAN_PAGES) $(MAN_DIR)/; \
		if command -v mandb >/dev/null 2>&1; then mandb -q; fi; \
	fi
	@if [ -d completions ]; then \
		echo "Installing bash completions to $(BASH_COMPLETION_DIR)..."; \
		mkdir -p $(BASH_COMPLETION_DIR) $(BASH_COMPLETION_LEGACY_DIR); \
		cp $(BASH_COMPLETIONS) $(BASH_COMPLETION_DIR)/; \
		cp $(BASH_COMPLETIONS) $(BASH_COMPLETION_LEGACY_DIR)/; \
		echo "Installing zsh completions to $(ZSH_COMPLETION_DIR)..."; \
		mkdir -p $(ZSH_COMPLETION_DIR); \
		cp $(ZSH_COMPLETIONS) $(ZSH_COMPLETION_DIR)/; \
	fi
	@echo "Installation completed successfully."

uninstall:
	@echo "Uninstalling from $(BIN_DIR)..."
	rm -f $(BIN_DIR)/pack $(BIN_DIR)/extract
	rm -f $(MAN_DIR)/extract.1 $(MAN_DIR)/pack.1
	rm -f $(BASH_COMPLETION_DIR)/extract $(BASH_COMPLETION_DIR)/pack
	rm -f $(BASH_COMPLETION_LEGACY_DIR)/extract $(BASH_COMPLETION_LEGACY_DIR)/pack
	rm -f $(ZSH_COMPLETION_DIR)/_extract $(ZSH_COMPLETION_DIR)/_pack
	@if command -v mandb >/dev/null 2>&1; then mandb -q; fi
	@echo "Uninstallation completed."

dist-pack:
	@mkdir -p $(DIST_DIR)
	@cp -r $(PACK_FILES) $(DIST_DIR)/
	@mkdir -p dist
	@tar -czf $(DIST_TAR) -C dist $(DIST_NAME)
	@echo "packed $(DIST_TAR)"

dist-extract:
	@test -f $(DIST_TAR) || (echo "missing $(DIST_TAR); run make dist-pack first" && exit 1)
	@rm -rf $(DIST_DIR)
	@tar -xzf $(DIST_TAR) -C dist
	@echo "extracted to $(DIST_DIR)"

release:
	@./scripts/release.sh

help:
	@echo "Available targets:"
	@echo "  all          - Build pack and extract binaries in $(BUILD_DIR)/"
	@echo "  install      - Install binaries, man pages, and completions"
	@echo "  uninstall    - Remove installed files"
	@echo "  dist-pack    - Create dist/$(DIST_NAME).tar.gz"
	@echo "  dist-extract - Extract release tarball to dist/"
	@echo "  release      - Publish GitHub release for current VERSION"
	@echo "  clean        - Remove $(BUILD_DIR)/"
