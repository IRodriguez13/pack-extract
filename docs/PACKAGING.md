# pack-extract — packaging checklist

> **Última verificación:** 2026-07-07  
> **Fuente de verdad:** `Makefile`, `debian/`, `tests/smoke-test.sh`, `pack.c`, `extract.c`

## Estado para paquetería oficial

| Requisito | Estado | Notas |
|-----------|--------|-------|
| Licencia GPLv3+ (`COPYING`, `debian/copyright`) | OK | Añadido en esta revisión |
| Binarios C (no bash en `/usr/bin`) | OK | `make` + `sudo make install PREFIX=/usr` o `.deb` |
| `pkg-config libarchive` en build | OK | Makefile |
| Man pages `pack(1)` / `extract(1)` | OK | Actualizar sección DEPENDENCIES → libarchive |
| Shell completions bash/zsh | OK | `debian/*.bash-completion` |
| `make check` (smoke) | OK | `tests/smoke-test.sh` |
| Skeleton Debian (`debian/`) | OK | `dpkg-buildpackage -us -uc` tras `debhelper` |
| CI (GitHub Actions) | OK | `.github/workflows/ci.yml` — `make check` + `dpkg-buildpackage` |
| ITP / RFS Debian | Falta | humano: nombre no colisiona con `extract` de otros paquetes |
| Paridad bash vs C (formatos) | Parcial | ver abajo |

## Build / verificación local

```bash
sudo apt-get install -y libarchive-dev build-essential debhelper-compat

cd ~/Escritorio/pack-extract
make clean all
make check
./scripts/verify-c-install.sh   # tras sudo ./install.sh

# Paquete .deb de prueba
dpkg-buildpackage -us -uc -b
```

## Dependencias runtime

- **Obligatoria:** `libarchive` (p. ej. `libarchive13t64` en Ubuntu 24.04).
- **No** requiere `tar`, `zip`, `7z` en PATH para la versión C (libarchive hace el I/O).

## Gaps conocidos (P1 antes de upload)

1. **`tar.br` / `br`:** en `pack.c` usaban `ARCHIVE_FILTER_PROGRAM` sin configurar programa; con libarchive ≥ 3.7 usar `ARCHIVE_FILTER_BROTLI` si está definido.
2. **Man pages:** tabla «Tool used» describe wrappers bash; la versión C debe decir **libarchive**.
3. **`dist-pack` tarball:** incluye `COPYING` y `debian/`.
4. **`pack.sh` / `extract.sh`:** mantener como referencia/legacy en repo; no instalar en `$PATH` (solo C).
5. **Autopkgtest:** opcional `debian/tests/smoke` invocando `make check`.
6. **Conflicto de nombres:** el binario `extract` es genérico; en Debian valorar `Provides`/`Conflicts` o renombrar a `pack-extract` / `px-extract` si el mantenedor lo exige.

## Publicación upstream (GitHub)

```bash
make dist-pack    # tarball fuente
make release      # gh release + asset (requiere gh auth)
```

## Fedora / RPM

Falta `.spec`; dependencias: `BuildRequires: libarchive-devel`, `%files` con `pack` y `extract`.
