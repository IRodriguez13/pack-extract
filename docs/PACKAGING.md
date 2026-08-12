# pack-extract — packaging checklist

> **Última verificación:** 2026-08-11  
> **Fuente de verdad:** `Makefile`, `debian/`, `PKGBUILD`, `tests/smoke-test.sh`, `pack.c`, `extract.c`, `Documentation/CLI.md`

## Estado para paquetería

| Requisito | Estado | Notas |
|-----------|--------|-------|
| Licencia GPLv3+ (`COPYING`, `debian/copyright`) | OK | |
| Binarios C (libarchive) | OK | `make` / `.deb` / `PKGBUILD` |
| `pkg-config libarchive` | OK | Makefile |
| Man pages `pack(1)` / `extract(1)` | OK | Backend = libarchive |
| Shell completions bash/zsh | OK | |
| `make check` (smoke + hardening) | OK | tree, abs path, symlink, SECURE_*, flags |
| Skeleton Debian (`debian/`) | OK | + `debian/tests/` autopkgtest |
| CI (GitHub Actions) | OK | `make check` + `dpkg-buildpackage` |
| AUR `PKGBUILD` | OK | en raíz; publicar en AUR es paso humano |
| ITP / RFS Debian oficial | Falta | humano; ver conflicto de nombre abajo |
| Legacy `pack.sh` / `extract.sh` | Eliminados | solo C en el árbol |

## Build / verificación local

```bash
sudo apt-get install -y libarchive-dev build-essential debhelper-compat

cd /path/to/pack-extract
make clean all
make check

# Paquete .deb de prueba
dpkg-buildpackage -us -uc -b
```

## AUR

| Paquete | Ruta | Compila |
|---------|------|---------|
| `pack-extract` | `PKGBUILD` (raíz) | Sí (fuente) |
| `pack-extract-bin` | `aur/pack-extract-bin/PKGBUILD` | No (tarball `*-linux-amd64.tar.gz` del release) |

```bash
# Tras publicar el release v1.5.5, fijar sha256sums (dejar de usar SKIP):
cd aur/pack-extract-bin
# bajar el asset, sha256sum, editar PKGBUILD, luego:
makepkg --printsrcinfo > .SRCINFO
makepkg -si
namcap PKGBUILD pack-extract-bin-*.pkg.tar.zst
```

Publicación en AUR (humano): cuenta en https://aur.archlinux.org/ + SSH key para `aur@aur.archlinux.org`, luego:

```bash
git clone ssh://aur@aur.archlinux.org/pack-extract-bin.git
# copiar PKGBUILD + .SRCINFO, commit, git push
```

Misma idea para `pack-extract` (fuente) si se publica el paquete que compila.

## Dependencias runtime

- **Obligatoria:** `libarchive`.
- No se requieren `tar`/`zip`/`7z` en PATH para la versión C.

## Nombre del binario `extract`

Se **mantiene** `extract` y `pack` por estabilidad CLI. En Debian oficial puede hacer falta `Conflicts`/`Provides` o renombrar si otro paquete reclama el nombre; documentar en ITP.

## Gaps restantes (no bloquean AUR / .deb en Releases)

1. Actualizar `sha256sums` del `PKGBUILD` al publicar el tag (ahora `SKIP` solo para desarrollo).
2. Fedora `.spec` (opcional).
3. Flags UX (`-C`, `-l`, `-q`) — P1 producto, fuera de este cierre de packaging.

## Publicación upstream (GitHub)

```bash
make dist-pack    # tarball fuente
make release      # gh release + assets (requiere gh auth)
```
