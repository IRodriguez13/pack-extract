#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

VERSION="$(tr -d '[:space:]' < VERSION)"
GH="${GH:-gh}"

if ! command -v "$GH" >/dev/null 2>&1; then
    echo "error: gh CLI not found (install: sudo apt install gh)" >&2
    exit 1
fi

if ! "$GH" auth status >/dev/null 2>&1; then
    echo "gh not authenticated. Run: gh auth login" >&2
    exit 1
fi

echo "Running smoke tests..."
make clean all check

echo "Building source tarball..."
make dist-pack

echo "Building prebuilt binary tarball..."
make dist-bin

if command -v dpkg-buildpackage >/dev/null 2>&1; then
    if dpkg -s debhelper >/dev/null 2>&1; then
        echo "Building .deb..."
        dpkg-buildpackage -us -uc -b
    else
        echo "warning: debhelper not installed; skipping .deb (install: sudo apt install debhelper-compat)" >&2
    fi
else
    echo "warning: dpkg-buildpackage not found; skipping .deb" >&2
fi

exec ./scripts/release-upload.sh
