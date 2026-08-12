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

# --- directory tree layout ---
echo "directory-tree..."
rm -rf treedir treedir.tar.gz
mkdir -p treedir/sub
echo "a-content" > treedir/a.txt
echo "b-content" > treedir/sub/b.txt
"$PACK" tar.gz treedir
rm -rf treedir
"$EXTRACT" treedir.tar.gz
test -f treedir/a.txt
test -f treedir/sub/b.txt
grep -q '^a-content$' treedir/a.txt
grep -q '^b-content$' treedir/sub/b.txt
# must not flatten to cwd
test ! -f a.txt
test ! -f b.txt
echo "  OK directory tree preserves prefix"

# --- absolute source path round-trip ---
echo "absolute-source..."
ABS_SRC="$WORKDIR/absdir"
rm -rf "$ABS_SRC" absdir absdir.tar.gz outdir
mkdir -p "$ABS_SRC/nested"
echo "abs-hello" > "$ABS_SRC/nested/file.txt"
"$PACK" -o absdir.tar.gz tar.gz "$ABS_SRC"
mkdir outdir
"$EXTRACT" -C outdir absdir.tar.gz
test -f outdir/absdir/nested/file.txt
grep -q '^abs-hello$' outdir/absdir/nested/file.txt
# archive must not contain absolute member paths
if tar -tzf absdir.tar.gz | grep -q '^/'; then
    echo "FAIL: archive contains absolute member paths"
    tar -tzf absdir.tar.gz
    exit 1
fi
echo "  OK absolute source stored as relative basename"

# --- symlink preservation ---
echo "symlink-preserve..."
rm -rf linkdir linkdir.tar.gz
mkdir linkdir
echo "target-data" > linkdir/real.txt
ln -s real.txt linkdir/alias.txt
"$PACK" tar.gz linkdir
rm -rf linkdir
"$EXTRACT" linkdir.tar.gz
test -f linkdir/real.txt
test -L linkdir/alias.txt
target="$(readlink linkdir/alias.txt)"
test "$target" = "real.txt"
grep -q '^target-data$' linkdir/alias.txt
echo "  OK symlink preserved"

# --- overwrite flags -n / -f ---
echo "overwrite-flags..."
rm -f sample.txt sample.txt.tar.gz
echo "v1" > sample.txt
"$PACK" tar.gz sample.txt
echo "v2" > sample.txt
"$EXTRACT" -n sample.txt.tar.gz >/dev/null
grep -q '^v2$' sample.txt
echo "  OK -n no-clobber keeps existing"

"$EXTRACT" -f sample.txt.tar.gz >/dev/null
grep -q '^v1$' sample.txt
echo "  OK -f force overwrites"

# --- -C directory ---
echo "chdir-extract..."
rm -rf cdest sample2.txt sample2.txt.tar.gz
echo "in-c" > sample2.txt
"$PACK" tar.gz sample2.txt
mkdir cdest
"$EXTRACT" -C cdest sample2.txt.tar.gz
test -f cdest/sample2.txt
grep -q '^in-c$' cdest/sample2.txt
test ! -f sample2.txt || true
# sample2.txt may still exist from pack cwd; ensure extract landed in cdest
echo "  OK -C extracts into directory"

# --- hostile symlink escape must fail (SECURE_SYMLINKS) ---
echo "secure-symlinks..."
python3 - <<'PY'
import os
import tarfile
import io

# Craft: symlink "evil" -> absolute dir outside cwd, then member "evil/pwned"
# SECURE_SYMLINKS should refuse writing through the symlink.
out = "evil-symlink.tar"
with tarfile.open(out, "w") as tf:
    info = tarfile.TarInfo(name="evil")
    info.type = tarfile.SYMTYPE
    info.linkname = "/tmp"
    tf.addfile(info)
    data = b"pwned-via-symlink\n"
    info2 = tarfile.TarInfo(name="evil/pwned-pack-extract")
    info2.size = len(data)
    tf.addfile(info2, io.BytesIO(data))
PY

# Do not use -f here: ARCHIVE_EXTRACT_UNLINK removes intermediate symlinks
# instead of refusing, which changes the failure mode.
rm -rf evil /tmp/pwned-pack-extract
if "$EXTRACT" evil-symlink.tar >/dev/null 2>&1; then
    echo "FAIL: extract accepted symlink-escape archive without error"
    exit 1
fi
if [ -f /tmp/pwned-pack-extract ]; then
    echo "FAIL: /tmp/pwned-pack-extract was created despite refusal"
    rm -f /tmp/pwned-pack-extract
    exit 1
fi
echo "  OK symlink escape rejected"

echo "smoke: OK"
