# Pack / Extract Utilities

Simple utilities for compressing and extracting files without having to remember flags or specific formats.

The objective is to unify the most common workflow:

- Extract anything
- Package in the format of your choice

## extract

Automatically extracts compressed archives by detecting the format.

### Usage
```bash
extract <archive>
```

### Examples
```bash
extract backup.tar.gz
extract project.zip
extract logs.tar.xz
extract data.7z
extract notes.txt.gz
```

### Supported Formats

| Format | Tool |
|--------|------|
| `.tar` | tar |
| `.tar.gz` / `.tgz` | tar + gzip |
| `.tar.xz` / `.txz` | tar + xz |
| `.tar.bz2` / `.tbz2` | tar + bzip2 |
| `.tar.zst` / `.tzst` | tar + zstd |
| `.tar.lz4` / `.tlz4` | tar + lz4 |
| `.tar.lz` | tar + lzip |
| `.gz` | gunzip |
| `.xz` | unxz |
| `.bz2` | bunzip2 |
| `.zst` | zstd |
| `.lz4` | lz4 |
| `.zip` | unzip |
| `.7z` | 7z |
| `.rar` | unrar |
| `.cpio` | cpio |

Content is extracted to the current directory.

## pack

Packages files or directories into the specified format.

### Usage
```bash
pack <format> <source>
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

### Supported Formats

**Archive formats** (files and directories):

| Format | Extension | Tool |
|--------|-----------|------|
| `tar` | `.tar` | tar |
| `tar.gz` | `.tar.gz` | tar + gzip |
| `tar.xz` | `.tar.xz` | tar + xz |
| `tar.bz2` | `.tar.bz2` | tar + bzip2 |
| `tar.zst` | `.tar.zst` | tar + zstd |
| `tar.lz4` | `.tar.lz4` | tar + lz4 |
| `tar.lz` | `.tar.lz` | tar + lzip |
| `zip` | `.zip` | zip |
| `7z` | `.7z` | 7z |

**Single-file compression** (files only):

| Format | Extension | Tool |
|--------|-----------|------|
| `gz` | `.gz` | gzip |
| `bz2` | `.bz2` | bzip2 |
| `xz` | `.xz` | xz |
| `zstd` | `.zst` | zstd |
| `lz4` | `.lz4` | lz4 |

The generated archive is created in the current directory with the source name.

## Installation

Run the install script:
```bash
# User install (~/.local/bin)
./install.sh

# System-wide install
sudo ./install.sh
```

Or manually:
```bash
chmod +x extract.sh pack.sh
sudo cp extract.sh /usr/local/bin/extract
sudo cp pack.sh /usr/local/bin/pack
```

Man pages are installed automatically by `install.sh`. To read them:
```bash
man extract
man pack
```

## Requirements

Both utilities are simple scripts that depend on standard system utilities.
Only the tools for the formats you use need to be installed:

- `tar`
- `gzip` / `gunzip`
- `bzip2` / `bunzip2`
- `xz` / `unxz`
- `zstd` (optional, for zstd support)
- `lz4` (optional, for lz4 support)
- `lzip` (optional, for lzip support)
- `zip` / `unzip`
- `7z` (optional, for 7z support)
- `unrar` (optional, for rar support)
- `cpio` (optional, for cpio support)
- `file` (for MIME type detection in extract)

## Notes

- Designed for local and manual use
- Not intended as a replacement for advanced archiving tools
- Provides clear error messages and operation feedback
- Follows standard Unix conventions for input/output
