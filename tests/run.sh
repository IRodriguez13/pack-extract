#!/usr/bin/env bash
# Run all pack-unpack regression tests.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
# shellcheck source=lib/common.sh
source "$ROOT/tests/lib/common.sh"

ensure_binaries

fail=0
run_one() {
    local script="$1"
    local name

    name="$(basename "$(dirname "$script")")/$(basename "$script")"
    echo "==> $name"
    if ! bash "$script"; then
        echo "FAIL: $name"
        fail=1
    fi
}

shopt -s nullglob
for script in \
    "$ROOT"/tests/pack/*.sh \
    "$ROOT"/tests/unpack/*.sh
do
    run_one "$script"
done

if [ "$fail" -ne 0 ]; then
    echo "tests: FAIL"
    exit 1
fi
echo "tests: OK"
