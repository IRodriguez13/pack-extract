#!/usr/bin/env bash
# Single-file RAW streams and outer-suffix naming.
set -euo pipefail
source "$(cd "$(dirname "$0")/.." && pwd)/lib/common.sh"
ensure_binaries

WORKDIR="$(make_workdir)"
trap 'rm -rf "$WORKDIR"' EXIT
cd "$WORKDIR"

for fmt in gz xz zstd; do
    rm -f sample.txt "sample.txt.${fmt}"
    echo "raw-${fmt}-payload" > sample.txt
    "$PACK" "$fmt" sample.txt
    rm -f sample.txt
    "$UNPACK" "sample.txt.${fmt}"
    grep -q "^raw-${fmt}-payload$" sample.txt
done

rm -f foo foo.tar foo.tar.gz
printf 'not-a-tar-payload\n' > foo.tar
"$PACK" gz foo.tar
rm -f foo.tar
"$UNPACK" foo.tar.gz
test -f foo.tar
test ! -e foo
grep -q '^not-a-tar-payload$' foo.tar

echo "  OK raw"
