#!/usr/bin/env bash
# Verify pack/unpack are ELF C binaries, not legacy bash wrappers.
set -euo pipefail

missing=0
for cmd in pack unpack; do
    if ! command -v "$cmd" >/dev/null 2>&1; then
        echo "missing: $cmd"
        missing=1
        continue
    fi
    path="$(command -v "$cmd")"
    if ! file "$path" | grep -q ELF; then
        echo "not ELF: $path"
        missing=1
    else
        echo "ok: $path"
        "$cmd" --version | head -1
    fi
done

if [ "$missing" -ne 0 ]; then
    echo "hint: make && make install   # or sudo make PREFIX=/usr INSTALL_EXTRACT_ALIAS=0 install"
    exit 1
fi
