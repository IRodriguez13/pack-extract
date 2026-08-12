#!/usr/bin/env bash
# Compatibility wrapper: delegates to tests/run.sh
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
exec "$ROOT/tests/run.sh"
