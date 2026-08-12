#!/usr/bin/env bash
# End-of-options: unpack -- -archive.tar
set -euo pipefail
source "$(cd "$(dirname "$0")/.." && pwd)/lib/common.sh"
ensure_binaries

WORKDIR="$(make_workdir)"
trap 'rm -rf "$WORKDIR"' EXIT
cd "$WORKDIR"

echo payload > sample.txt
"$PACK" tar sample.txt
# Portable leading-dash rename (BSD/macOS mv does not honor GNU --).
mv sample.txt.tar ./-archive.tar
rm -f sample.txt

"$UNPACK" -- -archive.tar
test -f sample.txt
grep -q '^payload$' sample.txt

if "$UNPACK" -archive.tar >/dev/null 2>&1; then
    echo "FAIL: leading-dash archive without -- should be rejected as option"
    exit 1
fi

echo "  OK end-of-options"
