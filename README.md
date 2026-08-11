# Pack / Extract Utilities

Simple utilities for compressing and extracting files without having to remember flags or specific formats.

The objective is to unify the most common workflow:

- Extract anything
- Package in the format of your choice

Implemented in **C** with [libarchive](https://www.libarchive.org/). Format detection and I/O go through libarchive (no per-format CLI flags).

## extract

Automatically extracts compressed archives by detecting the format.

### Usage
```bash
extract <archive>
extract --version
extract --help
```

### Examples
```bash
extract backup.tar.gz
extract project.zip
extract logs.tar.xz
extract data.7z
extract notes.txt.gz
```

### Supported formats (via libarchive)

Archives and compressed streams that libarchive can read, including:

| Kind | Examples |
|------|----------|
| tar + filter | `.tar`, `.tar.gz` / `.tgz`, `.tar.xz`, `.tar.bz2`, `.tar.zst`, `.tar.lz4`, `.tar.lz`, `.tar.lzo`, `.tar.br` |
| single-file | `.gz`, `.xz`, `.bz2`, `.zst`, `.lz4`, `.lzo`, `.br` |
| archive | `.zip`, `.7z`, and others supported by the linked libarchive |

Content is extracted to the current directory. Paths that are absolute or contain `..` are rejected (zip-slip). If a path already exists, `extract` asks to overwrite (`y`/`N`); Ctrl+C cancels. Non-interactive runs refuse overwrite on conflict.

## pack

Packages files or directories into the specified format.

### Usage
```bash
pack <format> <source>
pack --version
pack --help
```

### Examples
```bash
pack tar.gz project/
pack zip src/
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

**Single-file compression** (files only):

| Format | Extension | Backend |
|--------|-----------|---------|
| `gz` | `.gz` | libarchive |
| `bz2` | `.bz2` | libarchive |
| `xz` | `.xz` | libarchive |
| `zstd` | `.zstd` | libarchive |
| `lz4` | `.lz4` | libarchive |
| `lzo` | `.lzo` | libarchive |
| `br` | `.br` | libarchive (if built with brotli) |

The generated archive is created in the current directory with the source name plus the format extension (e.g. `report.txt.gz`).

## Installation

Prefer a **prebuilt** package so you do not need a compiler.

### Debian / Ubuntu (`.deb`, no compile)

```bash
VER=1.5.4
wget "https://github.com/IRodriguez13/pack-extract/releases/download/v${VER}/pack-extract_${VER}-1_amd64.deb"
sudo apt install "./pack-extract_${VER}-1_amd64.deb"
pack --version && extract --version
```

### Generic Linux (binary tarball, no compile)

```bash
VER=1.5.4
wget "https://github.com/IRodriguez13/pack-extract/releases/download/v${VER}/pack-extract-${VER}-linux-amd64.tar.gz"
tar -xzf "pack-extract-${VER}-linux-amd64.tar.gz"
cd "pack-extract-${VER}-linux-amd64"
./install.sh          # ~/.local/bin
# sudo ./install.sh   # /usr
```

Runtime: system `libarchive` (`sudo apt install libarchive13` or distro equivalent).

### Arch (AUR)

| Package | Compile? | Notes |
|---------|----------|--------|
| `pack-extract-bin` | No | Prebuilt from GitHub Releases (`aur/pack-extract-bin/PKGBUILD`) |
| `pack-extract` | Yes | Source build (`PKGBUILD` at repo root) |

```bash
# after packages are on AUR:
yay -S pack-extract-bin
# or from a clone of this repo:
cd aur/pack-extract-bin && makepkg -si
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

Packaging notes: [`docs/PACKAGING.md`](docs/PACKAGING.md). Contributors: [`CONTRIBUTORS.md`](CONTRIBUTORS.md).

Verify ELF binaries:

```bash
file "$(command -v extract)"
extract --version
```

Man pages:

```bash
man extract
man pack
```

## Requirements

**Build:** `gcc`, `make`, `pkg-config`, `libarchive` development package.

**Runtime:** `libarchive` (e.g. `libarchive13` / `libarchive13t64` on Debian/Ubuntu). Some filters may need codecs present in the system libarchive build (zstd, lz4, brotli, …).

## Notes

- Designed for local and manual use
- Not a replacement for advanced archiving tools (`7z` GUI, encrypted volumes, etc.)
- Binary names remain `pack` and `extract` (generic; see packaging notes if packaging for Debian official)
- Clear errors and a sorted list of extracted paths on success
