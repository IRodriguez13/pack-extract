#!/usr/bin/env bash
# Absolute / .. hardlink targets must be refused.
set -euo pipefail
source "$(cd "$(dirname "$0")/.." && pwd)/lib/common.sh"
ensure_binaries

WORKDIR="$(make_workdir)"
trap 'rm -rf "$WORKDIR"' EXIT
cd "$WORKDIR"

python3 - <<'PY'
import io
import tarfile

def write_hl(path, linkname):
    with tarfile.open(path, "w") as tf:
        data = b"anchor\n"
        info = tarfile.TarInfo(name="anchor.txt")
        info.size = len(data)
        tf.addfile(info, io.BytesIO(data))
        hl = tarfile.TarInfo(name="evil")
        hl.type = tarfile.LNKTYPE
        hl.linkname = linkname
        tf.addfile(hl)

write_hl("evil-hl-abs.tar", "/tmp/pwned-pack-unpack-hl")
write_hl("evil-hl-dotdot.tar", "../outside-hl")
PY

rm -f /tmp/pwned-pack-unpack-hl outside-hl
if "$UNPACK" evil-hl-abs.tar >/dev/null 2>&1; then
    echo "FAIL: absolute hardlink accepted"
    exit 1
fi
if [ -e /tmp/pwned-pack-unpack-hl ]; then
    echo "FAIL: absolute hardlink created target"
    rm -f /tmp/pwned-pack-unpack-hl
    exit 1
fi

if "$UNPACK" evil-hl-dotdot.tar >/dev/null 2>&1; then
    echo "FAIL: .. hardlink accepted"
    exit 1
fi

echo "  OK hardlink-security"
