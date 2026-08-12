#!/usr/bin/env bash
# Sparse file content round-trip. Hole preservation in the archive is NOT claimed.
set -euo pipefail
source "$(cd "$(dirname "$0")/.." && pwd)/lib/common.sh"
ensure_binaries

WORKDIR="$(make_workdir)"
trap 'rm -rf "$WORKDIR"' EXIT
cd "$WORKDIR"

# 1 MiB hole, then 16 bytes of payload at the end.
dd if=/dev/zero of=sparse.dat bs=1 count=0 seek=1048576 status=none
printf 'SPARSE-TAIL-OK\n' | dd of=sparse.dat bs=1 seek=1048576 conv=notrunc status=none

blocks_orig="$(stat -c '%b' sparse.dat 2>/dev/null || stat -f '%b' sparse.dat)"
size_orig="$(stat -c '%s' sparse.dat 2>/dev/null || stat -f '%z' sparse.dat)"
# On a sparse-capable FS, allocated 512-byte blocks should be << size/512.
if [ "$((blocks_orig * 512))" -ge "$size_orig" ]; then
    echo "  SKIP sparse (filesystem did not create a hole; blocks=$blocks_orig size=$size_orig)"
    exit 0
fi

cp sparse.dat sparse.dat.expected
"$PACK" tar sparse.dat
rm -f sparse.dat
"$UNPACK" sparse.dat.tar
cmp -s sparse.dat sparse.dat.expected

echo "  OK sparse (content round-trip; FS hole detected on input)"
