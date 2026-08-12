#!/usr/bin/env bash
# FIFO/socket/device must fail loudly, not silent-skip.
set -euo pipefail
source "$(cd "$(dirname "$0")/.." && pwd)/lib/common.sh"
ensure_binaries

WORKDIR="$(make_workdir)"
trap 'rm -rf "$WORKDIR"' EXIT
cd "$WORKDIR"

mkdir tree
echo ok > tree/a.txt
if ! mkfifo tree/pipe 2>/dev/null; then
    echo "  SKIP special-files (mkfifo unavailable)"
    exit 0
fi

err=0
msg="$("$PACK" tar tree 2>&1)" || err=$?
if [ "$err" -eq 0 ]; then
    echo "FAIL: pack should reject FIFO in tree"
    exit 1
fi
echo "$msg" | grep -qi 'unsupported file type'

echo "  OK special-files"
