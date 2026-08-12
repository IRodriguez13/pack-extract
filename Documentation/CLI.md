# pack / extract — CLI and security contract

> **Última verificación:** 2026-08-11
> **Fuente de verdad:** [`pack.c`](../pack.c), [`extract.c`](../extract.c), [`tests/smoke-test.sh`](../tests/smoke-test.sh)

## Interface

```text
pack  [-o output] <format> <source> [source...]
extract [-C dir] [-f|-n|-i] <archive>
```

Small, semantic verbs: **pack** creates; **extract** consumes. Format detection on extract is delegated to libarchive (`archive_read_support_format_all` / `support_filter_all`).

## Round-trip invariant

Everything `pack` produces with normal options must be consumable by `extract`:

| Rule | Behavior |
|------|----------|
| Member pathnames | Always **relative**; root = `basename(source)` (e.g. `/tmp/foo` → `foo/…`) |
| Directory trees | Children keep the parent prefix (`foo/a.txt`, not flattened `a.txt`) |
| Symlinks | Preserved via `lstat` + `AE_IFLNK` (not followed) |
| Hardlinks | Same `(st_dev, st_ino)` recorded as hardlink when the format supports it |
| Absolute sources | Allowed as input; stored without a leading `/` |

## extract security

Defense in depth:

1. Software checks reject absolute member paths and `..` components (zip-slip).
2. `archive_write_disk` options always include:
   - `ARCHIVE_EXTRACT_SECURE_SYMLINKS`
   - `ARCHIVE_EXTRACT_SECURE_NODOTDOT`
   - `ARCHIVE_EXTRACT_SECURE_NOABSOLUTEPATHS`

Textual pathname checks do **not** replace filesystem-level symlink redirection checks.

### Overwrite policy

| Mode | Flag | Behavior |
|------|------|----------|
| Default | (none) | TTY: prompt `y/N`; non-TTY: refuse conflict |
| Force | `-f` / `--force` | Overwrite; also sets `ARCHIVE_EXTRACT_UNLINK` |
| No-clobber | `-n` / `--no-clobber` | Skip existing paths |
| Interactive | `-i` / `--interactive` | Always prompt (still refuses on non-TTY) |

Only one of `-f`, `-n`, `-i` may be set. `-C DIR` changes directory before extraction; relative archive paths are resolved with `realpath` first.

**Note:** With `-f`, libarchive may unlink an intermediate symlink instead of aborting (`UNLINK` + `SECURE_SYMLINKS`). For untrusted archives prefer the default (no `-f`).

## pack errors

`archive_write_set_format`, `add_filter`, `open`, `header`, `data`, and `close` failures propagate as `EXIT_FAILURE`. Success is printed only when the archive closed cleanly.

## Verification

```bash
make clean all check
```

Smoke coverage includes format round-trips, directory trees, absolute sources, symlink preservation, overwrite flags, `-C`, zip-slip rejection, and symlink-escape refusal.
