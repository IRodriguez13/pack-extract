#!/bin/sh
# Install pack/unpack from a prebuilt binary tarball (no compiler needed).
set -e
ROOT="$(CDPATH= cd -- "$(dirname "$0")" && pwd)"

if [ "$(id -u)" -eq 0 ]; then
    PREFIX="${PREFIX:-/usr}"
else
    PREFIX="${PREFIX:-$HOME/.local}"
fi

BIN_DIR="$PREFIX/bin"
MAN_DIR="$PREFIX/share/man/man1"
mkdir -p "$BIN_DIR" "$MAN_DIR"

install -m 0755 "$ROOT/bin/pack" "$ROOT/bin/unpack" "$BIN_DIR/"
if [ -e "$ROOT/bin/extract" ]; then
    # Tarball may ship extract→unpack; skip if destination already exists (libextractor).
    if [ ! -e "$BIN_DIR/extract" ]; then
        cp -a "$ROOT/bin/extract" "$BIN_DIR/extract"
    else
        echo "note: leaving existing $BIN_DIR/extract in place" >&2
    fi
fi
install -m 0644 "$ROOT/share/man/man1/pack.1" "$ROOT/share/man/man1/unpack.1" "$MAN_DIR/"
if [ -f "$ROOT/share/man/man1/extract.1" ] && [ ! -e "$MAN_DIR/extract.1" ]; then
    install -m 0644 "$ROOT/share/man/man1/extract.1" "$MAN_DIR/"
fi

echo "Installed pack and unpack to $BIN_DIR"
