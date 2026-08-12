#!/usr/bin/env bash
# Round-trip basic formats and version strings.
set -euo pipefail
# shellcheck source=../lib/common.sh
source "$(cd "$(dirname "$0")/.." && pwd)/lib/common.sh"
ensure_binaries

WORKDIR="$(make_workdir)"
trap 'rm -rf "$WORKDIR"' EXIT
cd "$WORKDIR"

roundtrip() {
    local fmt="$1"
    local archive="sample.txt.${fmt}"

    rm -f sample.txt "$archive"
    echo "hello pack-unpack" > sample.txt
    "$PACK" "$fmt" sample.txt
    test -f "$archive"
    rm -f sample.txt
    "$UNPACK" "$archive"
    test -f sample.txt
    grep -q 'hello pack-unpack' sample.txt
    echo "  OK $fmt"
}

for fmt in tar.gz tar.xz tar.bz2 tar.zst zip 7z; do
    roundtrip "$fmt"
done

"$PACK" --version | grep -q pack-unpack
"$UNPACK" --version | grep -q pack-unpack
# -v is verbose, not version
if "$PACK" -v 2>&1 | grep -q 'pack (pack-unpack)'; then
    echo "FAIL: -v should not print version"
    exit 1
fi

# Directory tree layout
rm -rf treedir treedir.tar.gz
mkdir -p treedir/sub
echo "a-content" > treedir/a.txt
echo "b-content" > treedir/sub/b.txt
"$PACK" tar.gz treedir
rm -rf treedir
"$UNPACK" treedir.tar.gz
test -f treedir/a.txt
test -f treedir/sub/b.txt
grep -q '^a-content$' treedir/a.txt
grep -q '^b-content$' treedir/sub/b.txt
test ! -f a.txt
test ! -f b.txt

# Absolute source stored as relative basename
ABS_SRC="$WORKDIR/absdir"
rm -rf "$ABS_SRC" absdir absdir.tar.gz outdir
mkdir -p "$ABS_SRC/nested"
echo "abs-hello" > "$ABS_SRC/nested/file.txt"
"$PACK" -o absdir.tar.gz tar.gz "$ABS_SRC"
mkdir outdir
"$UNPACK" -C outdir absdir.tar.gz
test -f outdir/absdir/nested/file.txt
if tar -tzf absdir.tar.gz | grep -q '^/'; then
    echo "FAIL: archive contains absolute member paths"
    exit 1
fi

echo "  OK basic"
