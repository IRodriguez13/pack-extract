#!/usr/bin/env bash
# Helper to prepare AUR package trees locally (requires AUR SSH key configured).
# Usage:
#   ./scripts/prepare-aur.sh bin     # pack-extract-bin (no compile)
#   ./scripts/prepare-aur.sh source  # pack-extract (compiles)
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
MODE="${1:-bin}"

case "$MODE" in
  bin)
    NAME=pack-extract-bin
    SRC="$ROOT/aur/pack-extract-bin"
    ;;
  source|src)
    NAME=pack-extract
    SRC="$ROOT"
    ;;
  *)
    echo "usage: $0 {bin|source}" >&2
    exit 1
    ;;
esac

WORKDIR="${TMPDIR:-/tmp}/aur-${NAME}"
rm -rf "$WORKDIR"
mkdir -p "$WORKDIR"
cp "$SRC/PKGBUILD" "$WORKDIR/"
cd "$WORKDIR"

if command -v makepkg >/dev/null 2>&1; then
  makepkg --printsrcinfo > .SRCINFO
  echo "Wrote $WORKDIR/.SRCINFO"
  echo "Next (with AUR SSH key):"
  echo "  git clone ssh://aur@aur.archlinux.org/${NAME}.git"
  echo "  cp PKGBUILD .SRCINFO ${NAME}/"
  echo "  cd ${NAME} && git add PKGBUILD .SRCINFO && git commit -m 'Initial: ${NAME} ${NAME}' && git push"
else
  echo "makepkg not found (need Arch). PKGBUILD ready at $SRC/PKGBUILD"
  echo "On Arch: makepkg --printsrcinfo > .SRCINFO then push to aur@aur.archlinux.org:${NAME}.git"
fi
