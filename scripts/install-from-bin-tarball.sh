#!/usr/bin/env bash
# Install pack/extract from a prebuilt binary tarball (no compiler needed).
# Usage: ./install.sh          -> ~/.local
#        sudo ./install.sh     -> /usr (or PREFIX=...)
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

if [ "$(id -u)" -eq 0 ]; then
    PREFIX="${PREFIX:-/usr}"
else
    PREFIX="${PREFIX:-$HOME/.local}"
fi

BIN_DIR="$PREFIX/bin"
MAN_DIR="$PREFIX/share/man/man1"

mkdir -p "$BIN_DIR" "$MAN_DIR"
install -m 0755 "$ROOT/bin/pack" "$ROOT/bin/extract" "$BIN_DIR/"
install -m 0644 "$ROOT/share/man/man1/pack.1" "$ROOT/share/man/man1/extract.1" "$MAN_DIR/"

echo "Installed pack and extract to $BIN_DIR"
echo "Ensure $BIN_DIR is on your PATH."
command -v pack >/dev/null 2>&1 && pack --version || true
