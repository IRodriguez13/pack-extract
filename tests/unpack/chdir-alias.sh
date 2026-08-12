#!/usr/bin/env bash
# -C extract and extract argv0 alias.
set -euo pipefail
source "$(cd "$(dirname "$0")/.." && pwd)/lib/common.sh"
ensure_binaries

WORKDIR="$(make_workdir)"
trap 'rm -rf "$WORKDIR"' EXIT
cd "$WORKDIR"

echo "in-c" > sample2.txt
"$PACK" tar.gz sample2.txt
mkdir cdest
"$UNPACK" -C cdest sample2.txt.tar.gz
test -f cdest/sample2.txt
grep -q '^in-c$' cdest/sample2.txt

ln -sfn "$UNPACK" extract
echo "alias-payload" > alias.txt
"$PACK" gz alias.txt
rm -f alias.txt
./extract --version | grep -q '^extract (pack-unpack)'
./extract alias.txt.gz
test -f alias.txt
grep -q '^alias-payload$' alias.txt

echo "  OK chdir-alias"
