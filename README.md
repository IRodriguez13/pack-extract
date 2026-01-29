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
```

### Supported Formats
- `.tar`
- `.tar.gz`
- `.tar.xz`
- `.tar.bz2`
- `.zip`
- `.gz`
- `.7z`

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
```

### Supported Formats
- `tar`
- `tar.gz`
- `tar.xz`
- `tar.bz2`
- `zip`

The generated archive is created in the current directory with the source name.

## Installation

Make the scripts executable:
```bash
chmod +x extract.sh pack.sh
```

For system-wide usage, move to a directory in your PATH:
```bash
sudo cp extract.sh /usr/local/bin/extract
sudo cp pack.sh /usr/local/bin/pack
```

## Requirements

Both utilities are simple scripts that depend on standard system utilities:
- `tar`
- `zip`/`unzip`
- `gzip`
- `xz`
- `bzip2`
- `7z` (optional, for 7z support)

## Notes

- Designed for local and manual use
- Not intended as a replacement for advanced archiving tools
- Provides clear error messages and operation feedback
- Follows standard Unix conventions for input/output