#!/usr/bin/env bash
# pack .. must not create ../ members.
set -euo pipefail
source "$(cd "$(dirname "$0")/.." && pwd)/lib/common.sh"
ensure_binaries

WORKDIR="$(make_workdir)"
trap 'rm -rf "$WORKDIR"' EXIT

NEST="$WORKDIR/nest"
mkdir -p "$NEST/child"
echo nested > "$NEST/child/f.txt"
cd "$NEST/child"

"$PACK" -o "$WORKDIR/dotdot.tar" tar ..
if tar -tf "$WORKDIR/dotdot.tar" | grep -q '^\.\./'; then
    echo "FAIL: archive contains ../ members"
    tar -tf "$WORKDIR/dotdot.tar"
    exit 1
fi
# Root should be basename of resolved parent (nest)
tar -tf "$WORKDIR/dotdot.tar" | grep -q '^nest/'

echo "  OK dotdot"
