#!/usr/bin/env bash
# Recognized-but-corrupt container must not fall through to RAW.
set -euo pipefail
source "$(cd "$(dirname "$0")/.." && pwd)/lib/common.sh"
ensure_binaries

WORKDIR="$(make_workdir)"
trap 'rm -rf "$WORKDIR"' EXIT
cd "$WORKDIR"

# Valid tar → truncate the tar stream → gzip so the outer filter is valid
# but the tar container is broken after format identification.
echo 'hello-corrupt' > payload.txt
"$PACK" tar payload.txt
# Truncate tar mid-stream then wrap in gzip (valid compressor, broken payload).
dd if=payload.txt.tar of=broken.tar bs=1 count=120 status=none
gzip -c broken.tar > corrupt.tar.gz

rm -f payload.txt payload.txt.tar broken.tar
err=0
msg="$("$UNPACK" corrupt.tar.gz 2>&1)" || err=$?
if [ "$err" -eq 0 ]; then
    echo "FAIL: corrupt tar.gz should fail"
    echo "$msg"
    exit 1
fi
# Must not invent RAW member (corrupt.tar from outer .gz strip).
if [ -e corrupt.tar ] || [ -e payload.txt ] || [ -e broken.tar ]; then
    echo "FAIL: RAW fallback produced output from corrupt container"
    ls -la
    exit 1
fi

echo "  OK corrupt-container"
