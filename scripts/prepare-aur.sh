#!/usr/bin/env bash
# Prepare a local AUR package directory for upload.
# Usage:
#   ./scripts/prepare-aur.sh bin     # pack-unpack-bin (no compile)
#   ./scripts/prepare-aur.sh source  # pack-unpack (compiles)
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
MODE="${1:-}"

case "$MODE" in
bin)
    NAME=pack-unpack-bin
    SRC="$ROOT/aur/pack-unpack-bin"
    ;;
source)
    NAME=pack-unpack
    SRC="$ROOT"
    ;;
*)
    echo "usage: $0 bin|source" >&2
    exit 1
    ;;
esac

echo "Prepared package name: $NAME (from $SRC)"
echo "Copy PKGBUILD (+ .SRCINFO after makepkg --printsrcinfo) to your AUR clone."
