#!/usr/bin/env bash
# Verify pack/extract are ELF C binaries, not legacy bash wrappers.
set -euo pipefail

fail=0
for cmd in pack extract; do
    path="$(command -v "$cmd" || true)"
    if [ -z "$path" ]; then
        echo "FAIL: $cmd not in PATH"
        fail=1
        continue
    fi
    echo "== $cmd => $path =="
    file "$path"
    if file -b "$path" | grep -q 'shell script'; then
        echo "FAIL: $cmd is still a bash wrapper"
        fail=1
    elif ! file -b "$path" | grep -q 'ELF'; then
        echo "FAIL: $cmd is not ELF"
        fail=1
    else
        "$cmd" --version | head -1
        echo "OK"
    fi
    echo
done

if [ "$fail" -ne 0 ]; then
    echo "Fix: sudo apt-get install -y libarchive-dev build-essential"
    echo "     cd ~/Escritorio/pack-extract && sudo ./install.sh"
    exit 1
fi

echo "All checks passed."
