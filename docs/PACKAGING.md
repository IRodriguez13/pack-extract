# pack-unpack — packaging checklist

> **Última verificación:** 2026-08-11  
> **Fuente de verdad:** `Makefile`, `debian/`, `PKGBUILD`, `tests/smoke-test.sh`, `pack.c`, `unpack.c`, `Documentation/CLI.md`

## Estado para paquetería

| Requisito | Estado | Notas |
|-----------|--------|-------|
| Licencia GPLv3+ (`COPYING`, `debian/copyright`) | OK | |
| Binarios C (libarchive) | OK | `pack`, `unpack` |
| `pkg-config libarchive` | OK | Makefile |
| Man pages `pack(1)` / `unpack(1)` | OK | Backend = libarchive |
| Shell completions bash/zsh | OK | |
| `make check` (smoke + hardening) | OK | tree, abs path, symlink, SECURE_*, RAW, skip_file, alias |
| Skeleton Debian (`debian/`) | OK | package name `pack-unpack`; `INSTALL_EXTRACT_ALIAS=0` |
| CI (GitHub Actions) | OK | `make check` + `dpkg-buildpackage` |
| AUR `PKGBUILD` | OK | `pack-unpack` / `pack-unpack-bin`; no `/usr/bin/extract` |
| ITP / RFS Debian oficial | Falta | humano |
| Legacy `pack.sh` / `extract.sh` | Eliminados | solo C en el árbol |

## Build / verificación local

```bash
sudo apt-get install -y libarchive-dev build-essential debhelper-compat

cd /path/to/pack-extract   # git clone path; product name is pack-unpack
make clean all
make check

# Paquete .deb de prueba
dpkg-buildpackage -us -uc -b
```

Local install with optional `extract` alias (default):

```bash
make install                          # INSTALL_EXTRACT_ALIAS=1
# or:
make install INSTALL_EXTRACT_ALIAS=0  # unpack only
```

## AUR

| Paquete | Ruta | Compila |
|---------|------|---------|
| `pack-unpack` | `PKGBUILD` (raíz) | Sí (fuente) |
| `pack-unpack-bin` | `aur/pack-unpack-bin/PKGBUILD` | No (tarball `*-linux-amd64.tar.gz` del release) |

```bash
# Tras publicar el release, fijar sha256sums (dejar de usar SKIP):
cd aur/pack-unpack-bin
# bajar el asset, sha256sum, editar PKGBUILD, luego:
makepkg --printsrcinfo > .SRCINFO
makepkg -si
namcap PKGBUILD pack-unpack-bin-*.pkg.tar.zst
```

## Nombre del binario / alias

| Binario | Empaquetado |
|---------|-------------|
| `pack` | Siempre |
| `unpack` | Siempre (canónico) |
| `extract` | Solo alias local (`INSTALL_EXTRACT_ALIAS=1`); **no** en `.deb`/AUR |

Motivo: Debian/Arch ya tienen `/usr/bin/extract` de GNU libextractor.

## Dependencias runtime

`libarchive` (p. ej. `libarchive13` / `libarchive13t64` en Debian/Ubuntu).
