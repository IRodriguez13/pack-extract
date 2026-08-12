# pack-unpack — regression coverage (verified behavior)

> **Última verificación:** 2026-08-12
> **Fuente de verdad:** [`tests/run.sh`](../tests/run.sh), scripts bajo [`tests/pack/`](../tests/pack/), [`tests/unpack/`](../tests/unpack/); código [`src/pack.c`](../src/pack.c), [`src/unpack.c`](../src/unpack.c)

Solo se documentan propiedades ejercidas por `make check`. Afirmaciones no cubiertas por un script quedan fuera de esta tabla.

## Cómo ejecutar

```bash
make clean all check
# opcional:
make clean all check CC=gcc \
  CFLAGS='-O1 -g -Wall -Wextra -pedantic -fsanitize=address,undefined' \
  LDFLAGS='-fsanitize=address,undefined'
```

Harness: `tests/run.sh` (también `tests/smoke-test.sh` → delega al harness).

## Matriz test → propiedad

| Script | Propiedad verificada |
|--------|----------------------|
| [`pack/basic.sh`](../tests/pack/basic.sh) | Round-trip `tar.gz`/`tar.xz`/`tar.bz2`/`tar.zst`/`zip`/`7z`; `--version`; `-v` no es version; árbol conserva prefijo; source absoluto → miembros relativos |
| [`pack/quiet-verbose.sh`](../tests/pack/quiet-verbose.sh) | Éxito sin `-v` → stdout vacío; `-v` imprime el pathname del miembro |
| [`pack/dotdot.sh`](../tests/pack/dotdot.sh) | `pack tar ..` no crea miembros `../…`; root = basename del path resuelto |
| [`pack/special-files.sh`](../tests/pack/special-files.sh) | FIFO en el árbol → `EXIT_FAILURE` + mensaje `unsupported file type` (SKIP si `mkfifo` falla) |
| [`pack/self-output.sh`](../tests/pack/self-output.sh) | Rechazo output==source; archive dentro del árbol no se incluye; hardlink al destino no trunca datos; colisión de basenames multi-source |
| [`pack/hardlinks.sh`](../tests/pack/hardlinks.sh) | Symlink preservado; hardlink mismo inode tras unpack |
| [`pack/raw.sh`](../tests/pack/raw.sh) | Streams `gz`/`xz`/`zstd`; RAW `foo.tar.gz` → miembro `foo.tar` |
| [`pack/sparse.sh`](../tests/pack/sparse.sh) | Archivo sparse (hueco detectado en el FS) round-trip de **contenido** con `cmp` (SKIP si el FS no crea hueco). **No** afirma preservación de holes en el archive |
| [`unpack/overwrite.sh`](../tests/unpack/overwrite.sh) | Default no-TTY rechaza conflicto; `-n` no pisa; `-f` pisa |
| [`unpack/security.sh`](../tests/unpack/security.sh) | Zip-slip `/` y `..` rechazados; escape por symlink → fallo y no crea `/tmp/pwned-pack-unpack` |
| [`unpack/chdir-alias.sh`](../tests/unpack/chdir-alias.sh) | `-C` extrae en el directorio indicado; argv0 `extract` + `--version` |
| [`unpack/end-of-options.sh`](../tests/unpack/end-of-options.sh) | `unpack -- -archive.tar` funciona; sin `--` un nombre `-…` se rechaza como opción |
| [`unpack/corrupt-container.sh`](../tests/unpack/corrupt-container.sh) | `tar.gz` con tar truncado falla; no produce miembro RAW |
| [`unpack/hardlink-security.sh`](../tests/unpack/hardlink-security.sh) | Hardlink absoluto o `../…` rechazado |

## CI (workflow)

[`.github/workflows/ci.yml`](../.github/workflows/ci.yml) ejecuta `make check` en: Ubuntu gcc, Ubuntu clang+ASan/UBSan, Alpine musl, macOS, FreeBSD (`gmake`), más build `.deb`.

## Fuera de cobertura (no afirmar como “hecho” en docs)

- Preservación de holes sparse dentro del archive (hoy el cuerpo se copia con `read` secuencial; el test solo garantiza igualdad de bytes)
- Prompt interactivo `-i` con TTY real (solo política no-TTY / `-n` / `-f`)
- Codecs opcionales no presentes en el libarchive del host (`br`, etc.)
