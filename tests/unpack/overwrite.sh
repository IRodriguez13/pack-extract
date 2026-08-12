#!/usr/bin/env bash
# Overwrite policy: default refuse, -n skip, -f force.
set -euo pipefail
source "$(cd "$(dirname "$0")/.." && pwd)/lib/common.sh"
ensure_binaries

WORKDIR="$(make_workdir)"
trap 'rm -rf "$WORKDIR"' EXIT
cd "$WORKDIR"

echo "original" > sample.txt
"$PACK" tar.gz sample.txt
echo "changed" > sample.txt
if "$UNPACK" sample.txt.tar.gz >/dev/null 2>&1; then
    echo "FAIL: unpack overwrote without tty prompt"
    exit 1
fi
grep -q '^changed$' sample.txt

echo "v1" > sample.txt
"$PACK" tar.gz sample.txt
echo "v2" > sample.txt
"$UNPACK" -n sample.txt.tar.gz
grep -q '^v2$' sample.txt

"$UNPACK" -f sample.txt.tar.gz
grep -q '^v1$' sample.txt

echo "  OK overwrite"
