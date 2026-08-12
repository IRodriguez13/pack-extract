# Documentation

> **Última verificación:** 2026-08-12
> **Fuente de verdad:** código en `pack.c`, `unpack.c`; tests en `tests/`; manpages `man/pack.1`, `man/unpack.1`

Índice de documentación técnica de **pack-unpack**.

| Documento | Contenido |
|-----------|-----------|
| [CLI.md](CLI.md) | Interfaz, round-trip, seguridad, alias `extract` |
| [TESTING.md](TESTING.md) | Matriz script → propiedad (solo lo que `make check` prueba) |
| [OPERATION.md](OPERATION.md) | Operación / invariantes verificados (pre-RFC) |
| [../docs/PACKAGING.md](../docs/PACKAGING.md) | Empaquetado Debian / AUR / releases |
| [../README.md](../README.md) | Vista general e instalación |

Los manpages instalados (`man pack`, `man unpack`) son la referencia de usuario en el sistema. Para claims de comportamiento, priorizar [`TESTING.md`](TESTING.md) sobre prosa del README.
