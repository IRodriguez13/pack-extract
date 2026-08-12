#!/usr/bin/env bash
# Quiet success vs -v streaming; no success banner.
set -euo pipefail
source "$(cd "$(dirname "$0")/.." && pwd)/lib/common.sh"
ensure_binaries

WORKDIR="$(make_workdir)"
trap 'rm -rf "$WORKDIR"' EXIT
cd "$WORKDIR"

echo payload > sample.txt
out="$("$PACK" tar sample.txt 2>&1)"
if [ -n "$out" ]; then
    echo "FAIL: pack success should be silent, got: $out"
    exit 1
fi

rm -f sample.txt
out="$("$UNPACK" sample.txt.tar 2>&1)"
if [ -n "$out" ]; then
    echo "FAIL: unpack success should be silent, got: $out"
    exit 1
fi

echo payload2 > sample2.txt
out="$("$PACK" -v tar sample2.txt 2>&1)"
echo "$out" | grep -qx 'sample2.txt'

rm -f sample2.txt
out="$("$UNPACK" -v sample2.txt.tar 2>&1)"
echo "$out" | grep -qx 'sample2.txt'

echo "  OK quiet-verbose"
