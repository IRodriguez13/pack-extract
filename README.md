# Pack / Unpack Utilities

Simple utilities for compressing and extracting files without having to remember flags or specific formats.

The objective is to unify the most common workflow:

- Unpack anything
- Package in the format of your choice

Implemented in **C** with [libarchive](https://www.libarchive.org/). Format detection and I/O go through libarchive (no per-format CLI flags).

## unpack

Automatically unpacks compressed archives by detecting the format.

### Usage
```bash
unpack [-C dir] [-f|-n|-i] <archive>
unpack --version
unpack --help
```

| Flag | Meaning |
|------|---------|
| `-C DIR` | Change to `DIR` before unpacking |
| `-f` | Force overwrite |
| `-n` | Never overwrite (skip) |
| `-i` | Always prompt (needs a tty) |

Default overwrite policy: prompt on a tty; refuse conflicts when non-interactive. Only one of `-f`/`-n`/`-i` may be used.

Local installs may also provide `extract` as a symlink to `unpack` (`INSTALL_EXTRACT_ALIAS=1`). Distro packages omit that alias to avoid clashing with GNU libextractor.

### Examples
```bash
unpack backup.tar.gz
unpack -C /tmp/out project.zip
unpack -n data.7z
unpack -f logs.tar.xz
```

### Supported formats (via libarchive)

Archives and compressed streams that libarchive can read, including:

| Kind | Examples |
|------|----------|
| tar + filter | `.tar`, `.tar.gz` / `.tgz`, `.tar.xz`, `.tar.bz2`, `.tar.zst`, `.tar.lz4`, `.tar.lz`, `.tar.lzo`, `.tar.br` |
| single-file | `.gz`, `.xz`, `.bz2`, `.zst`, `.lz4`, `.lzo`, `.br` |
| archive | `.zip`, `.7z`, and others supported by the linked libarchive |

Content is unpacked to the current directory (or `-C`). Absolute paths and `..` components are rejected; libarchive `SECURE_*` extraction flags are enabled (symlink escape, absolute paths, `..`). See [`Documentation/CLI.md`](Documentation/CLI.md).

## pack

Packages files or directories into the specified format.

### Usage
```bash
pack [-o output] <format> <source> [source...]
pack --version
pack --help
```

Member paths inside the archive are always relative (`basename` of each source). Symlinks and hardlinks are preserved. Directory trees keep their root prefix.

### Examples
```bash
pack tar.gz project/
pack -o backup.tar.gz tar.gz /var/tmp/project
pack zip src/ extras/
pack tar.xz logs/
pack 7z backups/
pack tar.zst dataset/
pack gz report.txt
```

### Supported formats

**Archive formats** (files and directories):

| Format | Extension | Backend |
|--------|-----------|---------|
| `tar` | `.tar` | libarchive |
| `tar.gz` | `.tar.gz` | libarchive |
| `tar.xz` | `.tar.xz` | libarchive |
| `tar.bz2` | `.tar.bz2` | libarchive |
| `tar.zst` | `.tar.zst` | libarchive |
| `tar.lz4` | `.tar.lz4` | libarchive |
| `tar.lz` | `.tar.lz` | libarchive |
| `tar.lzo` | `.tar.lzo` | libarchive |
| `tar.br` | `.tar.br` | libarchive (if built with brotli) |
| `zip` | `.zip` | libarchive |
| `7z` | `.7z` | libarchive |

**Single-file compression** (exactly one regular file):

| Format | Extension | Backend |
|--------|-----------|---------|
| `gz` | `.gz` | libarchive |
| `bz2` | `.bz2` | libarchive |
| `xz` | `.xz` | libarchive |
| `zstd` | `.zstd` | libarchive |
| `lz4` | `.lz4` | libarchive |
| `lzo` | `.lzo` | libarchive |
| `br` | `.br` | libarchive (if built with brotli) |

Without `-o`, the output is `basename(first-source).format` in the current directory.

## Installation

Prefer a **prebuilt** package so you do not need a compiler.

### Debian / Ubuntu (`.deb`, no compile)

```bash
VER=1.5.7
wget "https://github.com/IRodriguez13/pack-unpack/releases/download/v${VER}/pack-unpack_${VER}-1_amd64.deb"
sudo apt install "./pack-unpack_${VER}-1_amd64.deb"
pack --version && unpack --version
```

### Generic Linux (binary tarball, no compile)

```bash
VER=1.5.7
wget "https://github.com/IRodriguez13/pack-unpack/releases/download/v${VER}/pack-unpack-${VER}-linux-amd64.tar.gz"
tar -xzf "pack-unpack-${VER}-linux-amd64.tar.gz"
cd "pack-unpack-${VER}-linux-amd64"
./install.sh          # ~/.local/bin
# sudo ./install.sh   # /usr
```

Runtime: system `libarchive` (`sudo apt install libarchive13` or distro equivalent).

### Arch (AUR)

| Package | Compile? | Notes |
|---------|----------|--------|
| `pack-unpack-bin` | No | Prebuilt from GitHub Releases (`aur/pack-unpack-bin/PKGBUILD`) |
| `pack-unpack` | Yes | Source build (`PKGBUILD` at repo root) |

```bash
# after packages are on AUR:
yay -S pack-unpack-bin
# or from a clone of this repo:
cd aur/pack-unpack-bin && makepkg -si
```

### From source

Build dependency: **libarchive** (`libarchive-dev` / `libarchive`).

```bash
sudo apt-get install -y libarchive-dev build-essential
./install.sh              # ~/.local/bin when non-root
sudo ./install.sh         # /usr when root
# or: make clean all check && sudo make PREFIX=/usr install
```

Local `.deb`: `dpkg-buildpackage -us -uc -b` (needs `debhelper-compat`).

Packaging notes: [`docs/PACKAGING.md`](docs/PACKAGING.md). CLI contract: [`Documentation/CLI.md`](Documentation/CLI.md). Contributors: [`CONTRIBUTORS.md`](CONTRIBUTORS.md).

Verify ELF binaries:

```bash
file "$(command -v unpack)"
unpack --version
```

Man pages:

```bash
man unpack
man pack
```

## Requirements

**Build:** `gcc`, `make`, `pkg-config`, `libarchive` development package.

**Runtime:** `libarchive` (e.g. `libarchive13` / `libarchive13t64` on Debian/Ubuntu). Some filters may need codecs present in the system libarchive build (zstd, lz4, brotli, …).

## Notes

- Designed for local and manual use
- Not a replacement for advanced archiving tools (`7z` GUI, encrypted volumes, etc.)
- Canonical binaries are `pack` and `unpack` (`extract` is an optional local alias) (generic; see packaging notes if packaging for Debian official)
- Clear errors and a sorted list of unpacked paths on success
