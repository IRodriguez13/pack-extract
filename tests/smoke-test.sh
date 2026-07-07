#!/usr/bin/env bash
# Smoke tests for pack/extract C binaries (run via: make check)
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

PACK="${PACK:-$ROOT/build/pack}"
EXTRACT="${EXTRACT:-$ROOT/build/extract}"

if [ ! -x "$PACK" ] || [ ! -x "$EXTRACT" ]; then
    echo "Building binaries..."
    make -C "$ROOT" all
fi

file "$PACK" | grep -q ELF || { echo "FAIL: pack is not ELF"; exit 1; }
file "$EXTRACT" | grep -q ELF || { echo "FAIL: extract is not ELF"; exit 1; }

WORKDIR="$(mktemp -d)"
trap 'rm -rf "$WORKDIR"' EXIT
cd "$WORKDIR"

echo "hello pack-extract" > sample.txt

"$PACK" tar.gz sample.txt
test -f sample.txt.tar.gz

rm -f sample.txt
"$EXTRACT" sample.txt.tar.gz
test -f sample.txt
grep -q hello sample.txt

"$PACK" --version | grep -q pack-extract
"$EXTRACT" --version | grep -q pack-extract

echo "smoke: OK"
