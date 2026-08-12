# pack-unpack — operación y contrato pre-RFC (hechos verificados)

> **Última verificación:** 2026-08-12
> **Fuente de verdad:** [`CLI.md`](CLI.md), [`TESTING.md`](TESTING.md), `make check`

Documento operativo corto. Nada aspiracional: solo comportamiento respaldado por tests listados en [`TESTING.md`](TESTING.md).

## Comandos

```text
pack   [-v] [-o OUT] FORMAT SOURCE...
unpack [-v] [-C DIR] [-f|-n|-i] ARCHIVE
```

- Éxito silencioso; `-v` lista miembros; versión solo con `--version`.
- Alias opcional `extract` → mismo binario que `unpack` (paquetes distro: `INSTALL_EXTRACT_ALIAS=0`).

## Invariantes comprobados

1. Miembros relativos; `pack … /abs/path` no mete paths absolutos.
2. `pack tar ..` no produce `../…` en el listing.
3. Round-trip de formatos de archive y RAW (`gz`/`xz`/`zstd`) con contenido intacto.
4. Symlinks y hardlinks preservados en tar.
5. FIFO en árbol → error (no archive “exitoso” incompleto).
6. Output==source rechazado; write atómico no trunca hardlink del destino.
7. unpack: zip-slip y symlink-escape rechazados; `-n`/`-f`/default no-TTY según [`TESTING.md`](TESTING.md).
8. Sparse: contenido round-trip cuando el FS crea hueco (sin claim de holes en el archive).

## Build / install

```bash
make clean all check
make PREFIX=/usr/local install   # caller elige PREFIX; default Makefile = /usr/local
```

## Releases

Versión canónica: archivo `VERSION`. Assets esperados (tras `make release`): source tarball, binary tarball linux-amd64, `.deb`. Detalle de empaquetado: [`../docs/PACKAGING.md`](../docs/PACKAGING.md).
