#!/usr/bin/env bash
# Smoke tests for pack/extract C binaries (run via: make check)
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

PACK="${PACK:-$ROOT/build/pack}"
EXTRACT="${EXTRACT:-$ROOT/build/extract}"

if [ ! -x "$PACK" ] || [ ! -x "$EXTRACT" ]; then
    echo "Building binaries..."
    make -C "$ROOT" all
fi

file "$PACK" | grep -q ELF || { echo "FAIL: pack is not ELF"; exit 1; }
file "$EXTRACT" | grep -q ELF || { echo "FAIL: extract is not ELF"; exit 1; }

WORKDIR="$(mktemp -d)"
trap 'rm -rf "$WORKDIR"' EXIT
cd "$WORKDIR"

roundtrip() {
    local fmt="$1"
    local archive="sample.txt.${fmt}"

    rm -f sample.txt "$archive"
    echo "hello pack-extract" > sample.txt
    "$PACK" "$fmt" sample.txt
    test -f "$archive"
    rm -f sample.txt
    "$EXTRACT" "$archive"
    test -f sample.txt
    grep -q 'hello pack-extract' sample.txt
    echo "  OK $fmt"
}

echo "round-trip formats..."
for fmt in tar.gz tar.xz tar.bz2 tar.zst zip 7z; do
    roundtrip "$fmt"
done

# zip-slip: absolute path and parent traversal must be rejected
python3 - <<'PY'
import zipfile

with zipfile.ZipFile("evil-abs.zip", "w") as z:
    z.writestr("/tmp/pack-extract-zipslip-abs", "pwned\n")
with zipfile.ZipFile("evil-dotdot.zip", "w") as z:
    z.writestr("../pack-extract-zipslip-out", "pwned\n")
PY

if "$EXTRACT" evil-abs.zip >/dev/null 2>&1; then
    echo "FAIL: extract accepted absolute path in zip"
    exit 1
fi
echo "  OK zip-slip absolute rejected"

if "$EXTRACT" evil-dotdot.zip >/dev/null 2>&1; then
    echo "FAIL: extract accepted .. path in zip"
    exit 1
fi
echo "  OK zip-slip .. rejected"

"$PACK" --version | grep -q pack-extract
"$EXTRACT" --version | grep -q pack-extract

# overwrite prompt: non-tty must refuse when target exists
echo "overwrite-guard..."
echo "original" > sample.txt
"$PACK" tar.gz sample.txt
echo "changed" > sample.txt
if "$EXTRACT" sample.txt.tar.gz >/dev/null 2>&1; then
    echo "FAIL: extract overwrote without tty prompt"
    exit 1
fi
grep -q '^changed$' sample.txt
echo "  OK non-tty refuse overwrite"

# overwrite with explicit y on a fake tty is hard in CI; skip + n path via yes pipe still non-tty.
# Documented behaviour: conflict on pipe → error (covered above).

echo "smoke: OK"
