#!/usr/bin/env bash
# Symlink and hardlink preservation.
set -euo pipefail
source "$(cd "$(dirname "$0")/.." && pwd)/lib/common.sh"
ensure_binaries

WORKDIR="$(make_workdir)"
trap 'rm -rf "$WORKDIR"' EXIT
cd "$WORKDIR"

mkdir linkdir
echo "target-data" > linkdir/real.txt
ln -s real.txt linkdir/alias.txt
"$PACK" tar.gz linkdir
rm -rf linkdir
"$UNPACK" linkdir.tar.gz
test -f linkdir/real.txt
test -L linkdir/alias.txt
test "$(readlink linkdir/alias.txt)" = "real.txt"
grep -q '^target-data$' linkdir/alias.txt

mkdir hldir
echo shared > hldir/a
ln hldir/a hldir/b
"$PACK" tar hldir
rm -rf hldir
"$UNPACK" hldir.tar
test -f hldir/a
test -f hldir/b
test "$(stat -c '%i' hldir/a 2>/dev/null || stat -f '%i' hldir/a)" = \
     "$(stat -c '%i' hldir/b 2>/dev/null || stat -f '%i' hldir/b)"

echo "  OK hardlinks"
