#!/usr/bin/env bash
# Shared helpers for pack-unpack regression tests.
# shellcheck shell=bash

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
PACK="${PACK:-$ROOT/build/pack}"
UNPACK="${UNPACK:-$ROOT/build/unpack}"

ensure_binaries() {
    if [ ! -x "$PACK" ] || [ ! -x "$UNPACK" ]; then
        echo "Building binaries..."
        make -C "$ROOT" all
    fi
}

# Create a fresh workdir under TMPDIR; prints path. Caller should trap cleanup.
make_workdir() {
    mktemp -d "${TMPDIR:-/tmp}/pack-unpack-test.XXXXXX"
}
