# pack / unpack — CLI and security contract

> **Última verificación:** 2026-08-11
> **Fuente de verdad:** [`pack.c`](../pack.c), [`unpack.c`](../unpack.c), [`tests/smoke-test.sh`](../tests/smoke-test.sh)

## Interface

```text
pack   [-o output] <format> <source> [source...]
unpack [-C dir] [-f|-n|-i] <archive>
```

Canonical verbs: **pack** creates; **unpack** consumes. Optional compatibility alias: **extract** → same binary as `unpack` (argv0-aware help/version). Distro packages omit the `extract` symlink so they do not clash with GNU libextractor.

Format detection on unpack uses libarchive `archive_read_support_format_all` first, then a **controlled RAW-only retry** when the first header fails (needed for `gz`/`xz`/`zstd`/… because `support_format_all` omits RAW, and enabling both at once can let mtree falsely bid).

## Single-file formats

`pack` writes them as `ARCHIVE_FORMAT_RAW` plus a compression filter (`ARCHIVE_FORMAT_EMPTY` has no writer). On unpack, RAW members named `data` are renamed by stripping a known suffix from the archive basename (`foo.txt.gz` → `foo.txt`).

## Output safety (pack)

| Guard | Behavior |
|-------|----------|
| Same path | Refuse `pack -o foo … foo` before open (would truncate the source) |
| Archive in tree | After open, `archive_write_set_skip_file(dev, ino)` plus a pre-header `dev`/`ino` skip so walking a directory that contains the output does not nest the archive inside itself |

## Round-trip invariant

Everything `pack` produces with normal options must be consumable by `unpack`:

| Rule | Behavior |
|------|----------|
| Member pathnames | Always **relative**; root = `basename(source)` (e.g. `/tmp/foo` → `foo/…`) |
| Directory trees | Children keep the parent prefix (`foo/a.txt`, not flattened `a.txt`) |
| Symlinks | Preserved via `lstat` + `AE_IFLNK` (not followed) |
| Hardlinks | Same `(st_dev, st_ino)` recorded as hardlink when the format supports it |
| Absolute sources | Allowed as input; stored without a leading `/` |

## unpack security

Defense in depth:

1. Software checks reject absolute member paths and `..` components (zip-slip).
2. `archive_write_disk` options always include:
   - `ARCHIVE_EXTRACT_SECURE_SYMLINKS`
   - `ARCHIVE_EXTRACT_SECURE_NODOTDOT`
   - `ARCHIVE_EXTRACT_SECURE_NOABSOLUTEPATHS`

### Overwrite policy

| Mode | Flag | Behavior |
|------|------|----------|
| Default | (none) | TTY: prompt `y/N`; non-TTY: refuse conflict |
| Force | `-f` / `--force` | Overwrite; also sets `ARCHIVE_EXTRACT_UNLINK` |
| No-clobber | `-n` / `--no-clobber` | Skip existing paths |
| Interactive | `-i` / `--interactive` | Always prompt (still refuses on non-TTY) |

Only one of `-f`, `-n`, `-i` may be set. `-C DIR` changes directory before unpacking; relative archive paths are resolved with `realpath` first.

**Note:** With `-f`, libarchive may unlink an intermediate symlink instead of aborting (`UNLINK` + `SECURE_SYMLINKS`). For untrusted archives prefer the default (no `-f`).

## pack errors

`archive_write_set_format`, `add_filter`, `open`, `header`, `data`, and `close` failures propagate as `EXIT_FAILURE`. Success is printed only when the archive closed cleanly.

## Verification

```bash
make clean all check
```

Smoke coverage includes format round-trips (archives **and** single-file `gz`/`xz`/`zstd`), directory trees, absolute sources, symlink preservation, overwrite flags, `-C`, zip-slip rejection, symlink-escape refusal, output==source refusal, skip_file self-archive exclusion, and the `extract` argv0 alias.

## Namespace

| Name | Role |
|------|------|
| `unpack` | Canonical command (always installed) |
| `extract` | Optional symlink/alias (`INSTALL_EXTRACT_ALIAS=1`); omitted from Debian/AUR packages |
| GNU libextractor `extract` | Unrelated metadata tool — distro packages avoid shipping our alias |
