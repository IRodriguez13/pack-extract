#!/usr/bin/env bash
# Install pack/extract C binaries (see Makefile). Bash wrappers: pack.sh / extract.sh (legacy).
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$ROOT"

need_libarchive() {
    if pkg-config --exists libarchive 2>/dev/null; then
        return 0
    fi
    if [ -f /usr/include/archive.h ]; then
        return 0
    fi
    echo "Error: libarchive development headers missing." >&2
    echo "  Debian/Ubuntu: sudo apt-get install -y libarchive-dev" >&2
    exit 1
}

if ! command -v make >/dev/null || ! command -v gcc >/dev/null; then
    echo "Error: make and gcc are required to build the C binaries." >&2
    exit 1
fi

need_libarchive

echo "Building pack and extract (C)..."
if [ "${EUID:-$(id -u)}" -eq 0 ] && [ -n "${SUDO_USER:-}" ]; then
    # Never leave root-owned artifacts in build/ (breaks dpkg-buildpackage as user).
    sudo -u "$SUDO_USER" make clean all
else
    make clean all
fi

if [ ! -x build/pack ] || [ ! -x build/extract ]; then
    echo "Error: build failed (expected build/pack and build/extract)." >&2
    exit 1
fi

file build/pack | grep -q 'ELF' || {
    echo "Error: build/pack is not an ELF binary." >&2
    exit 1
}

if [ "${EUID:-$(id -u)}" -eq 0 ]; then
    PREFIX="${PREFIX:-/usr}"
    echo "Installing C binaries to ${PREFIX}/bin ..."
    make PREFIX="$PREFIX" install
else
    echo "Installing C binaries to \$HOME/.local/bin ..."
    make PREFIX="$HOME/.local" install
    if ! echo ":$PATH:" | grep -q ":$HOME/.local/bin:"; then
        echo ""
        echo "Tip: add ~/.local/bin to PATH if not already:"
        echo '  export PATH="$HOME/.local/bin:$PATH"'
    fi
fi

echo ""
echo "Installed:"
command -v pack
command -v extract
file "$(command -v pack)" "$(command -v extract)"
pack --version | head -1
extract --version | head -1
