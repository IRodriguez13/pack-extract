#!/usr/bin/env bash
# Output safety: refuse same-path; skip archive inside tree; hardlink dest.
set -euo pipefail
source "$(cd "$(dirname "$0")/.." && pwd)/lib/common.sh"
ensure_binaries

WORKDIR="$(make_workdir)"
trap 'rm -rf "$WORKDIR"' EXIT
cd "$WORKDIR"

echo "do-not-clobber" > foo
if "$PACK" -o foo tar foo >/dev/null 2>&1; then
    echo "FAIL: pack -o foo tar foo should refuse"
    exit 1
fi
grep -q '^do-not-clobber$' foo

mkdir -p selfdir/sub
echo "keep-me" > selfdir/sub/a.txt
"$PACK" -o selfdir/backup.tar tar selfdir
test -f selfdir/backup.tar
if tar -tf selfdir/backup.tar | grep -q 'backup\.tar'; then
    echo "FAIL: archive included itself"
    exit 1
fi
tar -tf selfdir/backup.tar | grep -q 'selfdir/sub/a.txt'

rm -rf source backup.tar
mkdir -p source
echo secret > source/important.dat
ln source/important.dat backup.tar
"$PACK" -o backup.tar tar source/
grep -q '^secret$' source/important.dat
tar -tf backup.tar | grep -q 'source/important.dat'

# Basename collision
rm -rf a b
mkdir -p a b
echo 1 > a/config
echo 2 > b/config
if "$PACK" tar a/config b/config >/dev/null 2>&1; then
    echo "FAIL: colliding basenames should be refused"
    exit 1
fi

echo "  OK self-output"
