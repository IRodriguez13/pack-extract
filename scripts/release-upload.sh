#!/usr/bin/env bash
# Upload release assets for the current VERSION (used by release.yml and release.sh).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

VERSION="$(tr -d '[:space:]' < VERSION)"
TAG="v${VERSION}"
TAR="dist/pack-extract-${VERSION}.tar.gz"
GH="${GH:-gh}"
DEB="$(ls -1 ../*pack-extract_${VERSION}-*_amd64.deb 2>/dev/null | head -1 || true)"

if ! command -v "$GH" >/dev/null 2>&1; then
    echo "error: gh CLI not found" >&2
    exit 1
fi

if [ ! -f "$TAR" ]; then
    echo "error: missing $TAR (run make dist-pack)" >&2
    exit 1
fi

UPLOAD_ARGS=("$TAR")
if [ -n "$DEB" ] && [ -f "$DEB" ]; then
    UPLOAD_ARGS+=("$DEB")
else
    echo "warning: no .deb found; uploading tarball only" >&2
fi

NOTES="$(cat <<EOF
## pack-extract ${VERSION}

Utilidades \`pack\` y \`extract\` en C (libarchive) para empaquetar y extraer archivos sin recordar flags por formato.

### Instalación desde .deb (Ubuntu/Debian amd64)

\`\`\`bash
wget https://github.com/IRodriguez13/pack-extract/releases/download/${TAG}/pack-extract_${VERSION}-1_amd64.deb
sudo apt install ./pack-extract_${VERSION}-1_amd64.deb
pack --version && extract --version
\`\`\`

Si faltan dependencias: \`sudo apt-get install -f\`. Runtime: \`libarchive\`.

Reinstalar o actualizar **no requiere desinstalar** antes: el paquete reemplaza \`/usr/bin/pack\` y \`/usr/bin/extract\`.

### Instalación desde fuente

\`\`\`bash
tar -xzf pack-extract-${VERSION}.tar.gz
cd pack-extract-${VERSION}
sudo apt-get install -y libarchive-dev build-essential
make
sudo make install PREFIX=/usr
\`\`\`
EOF
)"

if "$GH" release view "$TAG" >/dev/null 2>&1; then
    "$GH" release upload "$TAG" "${UPLOAD_ARGS[@]}" --clobber
    "$GH" release edit "$TAG" --notes "$NOTES"
    echo "uploaded assets to existing release $TAG"
else
    "$GH" release create "$TAG" "${UPLOAD_ARGS[@]}" \
        --title "pack-extract ${VERSION}" \
        --notes "$NOTES"
    echo "created release $TAG"
fi
