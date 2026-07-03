#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

VERSION="$(tr -d '[:space:]' < VERSION)"
TAG="v${VERSION}"
TAR="dist/pack-extract-${VERSION}.tar.gz"
GH="${GH:-gh}"

if ! command -v "$GH" >/dev/null 2>&1; then
    echo "error: gh CLI not found (install: sudo apt install gh)" >&2
    exit 1
fi

make dist-pack

NOTES="$(cat <<EOF
## pack-extract ${VERSION}

Utilidades \`pack\` y \`extract\` en C (libarchive) para empaquetar y extraer archivos sin recordar flags por formato.

- \`pack -v\` / \`extract -v\` con bloque GPL y enlace al repositorio
- Man pages \`pack(1)\` y \`extract(1)\`
- Completions bash (con \`_init_completion\`) y zsh

### Instalación

\`\`\`bash
tar -xzf pack-extract-${VERSION}.tar.gz
cd pack-extract-${VERSION}
make
sudo make install
\`\`\`

Dependencia de compilación: \`libarchive-dev\`
EOF
)"

if ! "$GH" auth status >/dev/null 2>&1; then
    echo "gh not authenticated. Run: gh auth login" >&2
    exit 1
fi

if "$GH" release view "$TAG" --repo IRodriguez13/pack-extract >/dev/null 2>&1; then
    "$GH" release upload "$TAG" "$TAR" --repo IRodriguez13/pack-extract --clobber
    echo "uploaded asset to existing release $TAG"
else
    "$GH" release create "$TAG" "$TAR" \
        --repo IRodriguez13/pack-extract \
        --title "pack-extract ${VERSION}" \
        --notes "$NOTES"
    echo "created release $TAG"
fi
