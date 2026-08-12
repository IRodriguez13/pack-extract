# Makefile for pack-unpack (pack + unpack; optional extract alias)
# Builds and installs the C utilities

VERSION := $(shell cat VERSION)
CC ?= gcc
PKG_CONFIG ?= pkg-config

ARCHIVE_CFLAGS := $(shell $(PKG_CONFIG) --cflags libarchive 2>/dev/null)
ARCHIVE_LIBS := $(shell $(PKG_CONFIG) --libs libarchive 2>/dev/null)
ifeq ($(ARCHIVE_LIBS),)
ARCHIVE_LIBS := -larchive
endif

CFLAGS ?= -O2 -Wall -Wextra -pedantic
# Always append include/version flags even when the caller overrides CFLAGS.
override CFLAGS += -Iinclude -DPACK_UNPACK_VERSION=\"$(VERSION)\" $(ARCHIVE_CFLAGS)
# dpkg-buildpackage sets LDFLAGS (hardening/LTO); libraries go in LDLIBS.
LDFLAGS ?=
LDLIBS ?= $(ARCHIVE_LIBS)

BUILD_DIR = build
PACK = $(BUILD_DIR)/pack
UNPACK = $(BUILD_DIR)/unpack
PACK_SRC = pack.c
UNPACK_SRC = unpack.c

# Install extract → unpack symlink (0 for distro packages that must not clash
# with GNU libextractor's /usr/bin/extract).
INSTALL_EXTRACT_ALIAS ?= 1

# Caller chooses PREFIX (default /usr/local). Do not rewrite based on uid.
PREFIX ?= /usr/local
DESTDIR ?=

BIN_DIR = $(DESTDIR)$(PREFIX)/bin
MAN_DIR = $(DESTDIR)$(PREFIX)/share/man/man1
BASH_COMPLETION_DIR = $(DESTDIR)$(PREFIX)/share/bash-completion/completions
ZSH_COMPLETION_DIR = $(DESTDIR)$(PREFIX)/share/zsh/site-functions

MAN_PAGES = man/unpack.1 man/pack.1
BASH_COMPLETIONS = completions/bash/unpack completions/bash/pack
ZSH_COMPLETIONS = completions/zsh/_unpack completions/zsh/_pack
DIST_NAME = pack-unpack-$(VERSION)
DIST_DIR = dist/$(DIST_NAME)
DIST_TAR = dist/$(DIST_NAME).tar.gz
BIN_ARCH := $(shell uname -m | sed 's/x86_64/amd64/;s/aarch64/arm64/')
DIST_BIN_NAME = pack-unpack-$(VERSION)-linux-$(BIN_ARCH)
DIST_BIN_DIR = dist/$(DIST_BIN_NAME)
DIST_BIN_TAR = dist/$(DIST_BIN_NAME).tar.gz
PACK_FILES = VERSION COPYING Makefile README.md CONTRIBUTORS.md Documentation docs/PACKAGING.md PKGBUILD aur pack.c unpack.c include man completions debian install.sh scripts/verify-c-install.sh scripts/install-from-bin-tarball.sh tests .gitignore

.PHONY: all pack unpack clean install uninstall check help dist-pack dist-bin dist-unpack release

$(BUILD_DIR):
	mkdir -p $@

all: $(PACK) $(UNPACK)

pack-bin: $(PACK)

unpack-bin: $(UNPACK)

$(PACK): $(PACK_SRC) include/version.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS) $(LDLIBS)

$(UNPACK): $(UNPACK_SRC) include/version.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS) $(LDLIBS)

clean:
	@rm -rf $(BUILD_DIR) 2>/dev/null || { \
		echo "error: cannot remove $(BUILD_DIR)/ (files may be root-owned; run: sudo rm -rf $(BUILD_DIR))" >&2; \
		exit 1; \
	}

install: all
	@echo "Installing to $(BIN_DIR)..."
	mkdir -p $(BIN_DIR)
	cp $(PACK) $(BIN_DIR)/pack
	cp $(UNPACK) $(BIN_DIR)/unpack
	@if [ "$(INSTALL_EXTRACT_ALIAS)" = "1" ]; then \
		ln -sfn unpack $(BIN_DIR)/extract; \
		echo "Installed extract → unpack alias"; \
	fi
	@if [ -d man ]; then \
		echo "Installing man pages to $(MAN_DIR)..."; \
		mkdir -p $(MAN_DIR); \
		cp $(MAN_PAGES) $(MAN_DIR)/; \
		if [ "$(INSTALL_EXTRACT_ALIAS)" = "1" ]; then \
			printf '.so man1/unpack.1\n' > $(MAN_DIR)/extract.1; \
		fi; \
		if [ -z "$(DESTDIR)" ] && command -v mandb >/dev/null 2>&1; then mandb -q; fi; \
	fi
	@if [ -d completions ]; then \
		echo "Installing bash completions to $(BASH_COMPLETION_DIR)..."; \
		mkdir -p $(BASH_COMPLETION_DIR); \
		cp $(BASH_COMPLETIONS) $(BASH_COMPLETION_DIR)/; \
		if [ "$(INSTALL_EXTRACT_ALIAS)" = "1" ]; then \
			cp completions/bash/extract $(BASH_COMPLETION_DIR)/; \
		fi; \
		echo "Installing zsh completions to $(ZSH_COMPLETION_DIR)..."; \
		mkdir -p $(ZSH_COMPLETION_DIR); \
		cp $(ZSH_COMPLETIONS) $(ZSH_COMPLETION_DIR)/; \
		if [ "$(INSTALL_EXTRACT_ALIAS)" = "1" ]; then \
			cp completions/zsh/_extract $(ZSH_COMPLETION_DIR)/; \
		fi; \
	fi
	@echo "Installation completed successfully."

check: all
	@chmod +x tests/run.sh tests/smoke-test.sh tests/pack/*.sh tests/unpack/*.sh
	@./tests/run.sh

uninstall:
	@echo "Uninstalling from $(BIN_DIR)..."
	rm -f $(BIN_DIR)/pack $(BIN_DIR)/unpack $(BIN_DIR)/extract
	rm -f $(MAN_DIR)/unpack.1 $(MAN_DIR)/pack.1 $(MAN_DIR)/extract.1
	rm -f $(BASH_COMPLETION_DIR)/unpack $(BASH_COMPLETION_DIR)/pack $(BASH_COMPLETION_DIR)/extract
	rm -f $(ZSH_COMPLETION_DIR)/_unpack $(ZSH_COMPLETION_DIR)/_pack $(ZSH_COMPLETION_DIR)/_extract
	@if command -v mandb >/dev/null 2>&1; then mandb -q; fi
	@echo "Uninstallation completed."

dist-pack:
	@mkdir -p $(DIST_DIR)
	@cp -r $(PACK_FILES) $(DIST_DIR)/
	@mkdir -p dist
	@tar -czf $(DIST_TAR) -C dist $(DIST_NAME)
	@echo "packed $(DIST_TAR)"

# Prebuilt binaries for install-without-compile (GitHub Releases / AUR -bin).
dist-bin: all
	@rm -rf $(DIST_BIN_DIR)
	@mkdir -p $(DIST_BIN_DIR)/bin $(DIST_BIN_DIR)/share/man/man1
	@cp $(PACK) $(UNPACK) $(DIST_BIN_DIR)/bin/
	@ln -sfn unpack $(DIST_BIN_DIR)/bin/extract
	@cp $(MAN_PAGES) $(DIST_BIN_DIR)/share/man/man1/
	@printf '.so man1/unpack.1\n' > $(DIST_BIN_DIR)/share/man/man1/extract.1
	@cp scripts/install-from-bin-tarball.sh $(DIST_BIN_DIR)/install.sh
	@chmod +x $(DIST_BIN_DIR)/install.sh $(DIST_BIN_DIR)/bin/pack $(DIST_BIN_DIR)/bin/unpack
	@mkdir -p dist
	@tar -czf $(DIST_BIN_TAR) -C dist $(DIST_BIN_NAME)
	@echo "packed $(DIST_BIN_TAR)"

dist-unpack:
	@test -f $(DIST_TAR) || (echo "missing $(DIST_TAR); run make dist-pack first" && exit 1)
	@rm -rf $(DIST_DIR)
	@tar -xzf $(DIST_TAR) -C dist
	@echo "unpacked to $(DIST_DIR)"

release:
	@./scripts/release.sh

help:
	@echo "Available targets:"
	@echo "  all          - Build pack and unpack binaries in $(BUILD_DIR)/"
	@echo "  check        - Run regression tests (tests/run.sh)"
	@echo "  install      - Install binaries, man pages, and completions (PREFIX=$(PREFIX); default /usr/local)"
	@echo "                 INSTALL_EXTRACT_ALIAS=$(INSTALL_EXTRACT_ALIAS) (extract→unpack symlink)"
	@echo "  uninstall    - Remove installed files"
	@echo "  dist-pack    - Create source dist/$(DIST_NAME).tar.gz"
	@echo "  dist-bin     - Create prebuilt dist/$(DIST_BIN_NAME).tar.gz"
	@echo "  dist-unpack  - Unpack source release tarball to dist/"
	@echo "  release      - Publish GitHub release for current VERSION"
	@echo "  clean        - Remove $(BUILD_DIR)/"
