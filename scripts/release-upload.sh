#!/usr/bin/env bash
# Upload release assets for the current VERSION (used by release.yml and release.sh).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

VERSION="$(tr -d '[:space:]' < VERSION)"
TAG="v${VERSION}"
TAR="dist/pack-unpack-${VERSION}.tar.gz"
BIN_TAR="$(ls -1 dist/pack-unpack-${VERSION}-linux-*.tar.gz 2>/dev/null | head -1 || true)"
GH="${GH:-gh}"
DEB="$(ls -1 ../*pack-unpack_${VERSION}-*_amd64.deb 2>/dev/null | head -1 || true)"

if ! command -v "$GH" >/dev/null 2>&1; then
    echo "error: gh CLI not found" >&2
    exit 1
fi

if [ ! -f "$TAR" ]; then
    echo "error: missing $TAR (run make dist-pack)" >&2
    exit 1
fi

UPLOAD_ARGS=("$TAR")
if [ -n "$BIN_TAR" ] && [ -f "$BIN_TAR" ]; then
    UPLOAD_ARGS+=("$BIN_TAR")
else
    echo "warning: no prebuilt binary tarball; run make dist-bin" >&2
fi
if [ -n "$DEB" ] && [ -f "$DEB" ]; then
    UPLOAD_ARGS+=("$DEB")
else
    echo "warning: no .deb found; uploading without .deb" >&2
fi

NOTES="$(cat <<EOF
## pack-unpack ${VERSION}

CLI \`pack\` / \`unpack\` in C (libarchive). Optional local alias: \`extract\` → \`unpack\`.

### Install without compiling (Debian / Ubuntu amd64)

\`\`\`bash
wget https://github.com/IRodriguez13/pack-extract/releases/download/${TAG}/pack-unpack_${VERSION}-1_amd64.deb
sudo apt install ./pack-unpack_${VERSION}-1_amd64.deb
pack --version && unpack --version
\`\`\`

Runtime dependency: \`libarchive\` (pulled in by apt).

### Install without compiling (generic Linux amd64 tarball)

\`\`\`bash
wget https://github.com/IRodriguez13/pack-extract/releases/download/${TAG}/pack-unpack-${VERSION}-linux-amd64.tar.gz
tar -xzf pack-unpack-${VERSION}-linux-amd64.tar.gz
cd pack-unpack-${VERSION}-linux-amd64
./install.sh          # ~/.local/bin (includes extract→unpack alias)
# sudo ./install.sh   # /usr
\`\`\`

Needs \`libarchive\` installed on the system (\`libarchive13\` / \`libarchive\` package).

### Arch (AUR)

- \`pack-unpack-bin\` — prebuilt (no compile); see \`aur/pack-unpack-bin/PKGBUILD\`
- \`pack-unpack\` — build from source (\`PKGBUILD\` at repo root)

### From source

\`\`\`bash
tar -xzf pack-unpack-${VERSION}.tar.gz
cd pack-unpack-${VERSION}
sudo apt-get install -y libarchive-dev build-essential   # or pacman -S libarchive base-devel
make && sudo make install PREFIX=/usr INSTALL_EXTRACT_ALIAS=0
\`\`\`
EOF
)"

if "$GH" release view "$TAG" >/dev/null 2>&1; then
    "$GH" release upload "$TAG" "${UPLOAD_ARGS[@]}" --clobber
    "$GH" release edit "$TAG" --notes "$NOTES"
    echo "uploaded assets to existing release $TAG"
else
    "$GH" release create "$TAG" "${UPLOAD_ARGS[@]}" \
        --title "pack-unpack ${VERSION}" \
        --notes "$NOTES"
    echo "created release $TAG"
fi
