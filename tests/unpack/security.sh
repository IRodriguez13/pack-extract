#!/usr/bin/env bash
# Zip-slip and SECURE_SYMLINKS refusal.
set -euo pipefail
source "$(cd "$(dirname "$0")/.." && pwd)/lib/common.sh"
ensure_binaries

WORKDIR="$(make_workdir)"
trap 'rm -rf "$WORKDIR"' EXIT
cd "$WORKDIR"

python3 - <<'PY'
import zipfile
with zipfile.ZipFile("evil-abs.zip", "w") as z:
    z.writestr("/tmp/pack-unpack-zipslip-abs", "pwned\n")
with zipfile.ZipFile("evil-dotdot.zip", "w") as z:
    z.writestr("../pack-unpack-zipslip-out", "pwned\n")
PY

if "$UNPACK" evil-abs.zip >/dev/null 2>&1; then
    echo "FAIL: accepted absolute path"
    exit 1
fi
if "$UNPACK" evil-dotdot.zip >/dev/null 2>&1; then
    echo "FAIL: accepted .. path"
    exit 1
fi

python3 - <<'PY'
import tarfile, io
out = "evil-symlink.tar"
with tarfile.open(out, "w") as tf:
    info = tarfile.TarInfo(name="evil")
    info.type = tarfile.SYMTYPE
    info.linkname = "/tmp"
    tf.addfile(info)
    data = b"pwned-via-symlink\n"
    info2 = tarfile.TarInfo(name="evil/pwned-pack-unpack")
    info2.size = len(data)
    tf.addfile(info2, io.BytesIO(data))
PY

rm -rf evil /tmp/pwned-pack-unpack
if "$UNPACK" evil-symlink.tar >/dev/null 2>&1; then
    echo "FAIL: accepted symlink-escape"
    exit 1
fi
if [ -f /tmp/pwned-pack-unpack ]; then
    echo "FAIL: /tmp/pwned-pack-unpack created"
    rm -f /tmp/pwned-pack-unpack
    exit 1
fi

echo "  OK security"
