# pack / unpack — CLI and security contract

> **Última verificación:** 2026-08-12
> **Fuente de verdad:** [`src/pack.c`](../src/pack.c), [`src/unpack.c`](../src/unpack.c), [`tests/run.sh`](../tests/run.sh)

## Interface

```text
pack   [-v] [-o output] [--] <format> <source> [source...]
unpack [-v] [-p] [-C dir] [-f|-n|-i] [--] <archive>
```

Canonical verbs: **pack** creates; **unpack** consumes. Optional compatibility alias: **extract** → same binary as `unpack` (argv0-aware help/version). Distro packages omit the `extract` symlink so they do not clash with GNU libextractor.

Success is **silent**. `-v` / `--verbose` lists members as they are processed. Version is **only** `--version` (`-v` is not an alias for version). End-of-options `--` is supported (e.g. `unpack -- -file.tar`).

Default unpack restores content, structure, links, and mtime under the process umask, plus security/atomic flags. `-p` / `--preserve` adds full `PERM`/`ACL`/`FFLAGS`.

Format detection on unpack uses libarchive `archive_read_support_format_all` first, then a **controlled RAW-only retry** only when no solid container (tar/cpio/zip/7z/…) was identified — so a corrupt recognized tar.gz fails instead of becoming a RAW gunzip. (Bare `.gz` may falsely bid mtree under `format_all`; that path still retries RAW.)

Sources live under [`src/`](../src/).
## Single-file formats

`pack` writes them as `ARCHIVE_FORMAT_RAW` plus a compression filter (`ARCHIVE_FORMAT_EMPTY` has no writer). On unpack, RAW members named `data` are renamed by stripping only a simple outer compression suffix from the archive basename (`foo.txt.gz` → `foo.txt`; `foo.tar.gz` → `foo.tar`). Compound suffixes like `.tar.gz` are not peeled to a bare stem — if `format_all` had recognized a container, unpack would not be on the RAW path.

## Output safety (pack)

| Guard | Behavior |
|-------|----------|
| Same path | Refuse `pack -o foo … foo` before open (would truncate the source) |
| Basename collision | Refuse when two sources share the same `basename` archive root (e.g. `a/config` and `b/config`) |
| `.` / `..` roots | `realpath` then basename; never store `../…` members |
| Atomic write | `mkstemp` → `fchmod(0666&~umask)` → `archive_write_open_fd` → close fd → `rename` |
| Archive in tree | `archive_write_set_skip_file` on the temp inode; final destination skipped by `realpath` |
| Unsupported types | FIFO/socket/device → error (no silent skip) |

Trees are walked with `archive_read_disk` (physical symlinks). Regular file bodies use `open` + `fstat` on the same fd; short reads are errors.

## Round-trip invariant

Everything `pack` produces with normal options must be consumable by `unpack`:

| Rule | Behavior |
|------|----------|
| Member pathnames | Always **relative**; root = `basename(source)` (after resolving `.`/`..`) |
| Directory trees | Children keep the parent prefix (`foo/a.txt`, not flattened `a.txt`) |
| Symlinks | Preserved (not followed) |
| Hardlinks | Same `(st_dev, st_ino)` recorded as hardlink when the format supports it |
| Absolute sources | Allowed as input; stored without a leading `/` |

## unpack security

Defense in depth:

1. Software checks reject absolute member paths and `..` components (zip-slip).
2. `archive_write_disk` options always include:
   - `ARCHIVE_EXTRACT_TIME`
   - `ARCHIVE_EXTRACT_SECURE_SYMLINKS`
   - `ARCHIVE_EXTRACT_SECURE_NODOTDOT`
   - `ARCHIVE_EXTRACT_SECURE_NOABSOLUTEPATHS`
   - `ARCHIVE_EXTRACT_SAFE_WRITES`
3. Default and `-n` also set `ARCHIVE_EXTRACT_NO_OVERWRITE`.
4. `-p` / `--preserve` adds `PERM` | `ACL` | `FFLAGS` (exhaustive metadata; not default).
5. After all entries, `archive_write_close(ext)` is checked before `archive_write_free` (deferred directory modes).
6. Hardlink targets are subject to the same absolute/`..` safety checks as pathnames.
### Overwrite policy

| Mode | Flag | Behavior |
|------|------|----------|
| Default | (none) | TTY (stdin+stderr): prompt `y/N`; non-TTY: refuse conflict |
| Force | `-f` / `--force` | Overwrite; also sets `ARCHIVE_EXTRACT_UNLINK` |
| No-clobber | `-n` / `--no-clobber` | Skip existing paths |
| Interactive | `-i` / `--interactive` | Always prompt (still refuses on non-TTY) |

Only one of `-f`, `-n`, `-i` may be set. `-C DIR` changes directory before unpacking; relative archive paths are resolved with `realpath` first.

**Note:** With `-f`, libarchive may unlink an intermediate symlink instead of aborting (`UNLINK` + `SECURE_SYMLINKS`). For untrusted archives prefer the default (no `-f`).

## pack errors

`archive_write_set_format`, `add_filter`, `open_fd`, `header`, `data`, and `close` failures propagate as `EXIT_FAILURE`. Success produces no stdout unless `-v` was given.

## Verification

```bash
make clean all check
```

Matriz completa script → propiedad: [`TESTING.md`](TESTING.md). Harness: `tests/run.sh`.

## Namespace

| Name | Role |
|------|------|
| `unpack` | Canonical command (always installed) |
| `extract` | Optional symlink/alias (`INSTALL_EXTRACT_ALIAS=1`); omitted from Debian/AUR packages |
| GNU libextractor `extract` | Unrelated metadata tool — distro packages avoid shipping our alias |
