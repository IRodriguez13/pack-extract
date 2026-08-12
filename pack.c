/**
 * pack - Unified compression tool rewrite in C
 * Copyright (C) 2026 Iván Ezequiel Rodriguez
 * License: GPLv3+
 *
 * Usage: pack [-v] [-o output] <format> <source> [source...]
 *   format: tar, tar.gz, tar.xz, tar.bz2, tar.zst, zip, 7z, ...
 *   source: file or directory to pack (archive pathnames are always relative)
 *   -o:    output filename (optional; default basename(first).format)
 *   -v:    verbose (list members as they are packed)
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <libgen.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <archive.h>
#include <archive_entry.h>
#include "version.h"

typedef struct
{
    const char *format;
    int format_id;
    int filter_id;
} PackFormat;

typedef struct HardlinkNode
{
    dev_t dev;
    ino_t ino;
    char *pathname;
    struct HardlinkNode *next;
} HardlinkNode;

typedef struct
{
    HardlinkNode *head;
} HardlinkMap;

static int g_verbose = 0;

static const PackFormat formats[] = {
    {"tar",     ARCHIVE_FORMAT_TAR, 0},
    {"tar.gz",  ARCHIVE_FORMAT_TAR, ARCHIVE_FILTER_GZIP},
    {"tar.bz2", ARCHIVE_FORMAT_TAR, ARCHIVE_FILTER_BZIP2},
    {"tar.xz",  ARCHIVE_FORMAT_TAR, ARCHIVE_FILTER_XZ},
    {"tar.zst", ARCHIVE_FORMAT_TAR, ARCHIVE_FILTER_ZSTD},
    {"tar.lz4", ARCHIVE_FORMAT_TAR, ARCHIVE_FILTER_LZ4},
    {"tar.lz",  ARCHIVE_FORMAT_TAR, ARCHIVE_FILTER_LZIP},
    {"tar.lzo", ARCHIVE_FORMAT_TAR, ARCHIVE_FILTER_LZOP},
#if defined(ARCHIVE_FILTER_BROTLI)
    {"tar.br",  ARCHIVE_FORMAT_TAR, ARCHIVE_FILTER_BROTLI},
#endif
    {"gz",      ARCHIVE_FORMAT_RAW, ARCHIVE_FILTER_GZIP},
    {"bz2",     ARCHIVE_FORMAT_RAW, ARCHIVE_FILTER_BZIP2},
    {"xz",      ARCHIVE_FORMAT_RAW, ARCHIVE_FILTER_XZ},
    {"zstd",    ARCHIVE_FORMAT_RAW, ARCHIVE_FILTER_ZSTD},
    {"lz4",     ARCHIVE_FORMAT_RAW, ARCHIVE_FILTER_LZ4},
    {"lzo",     ARCHIVE_FORMAT_RAW, ARCHIVE_FILTER_LZOP},
#if defined(ARCHIVE_FILTER_BROTLI)
    {"br",      ARCHIVE_FORMAT_RAW, ARCHIVE_FILTER_BROTLI},
#endif
    {"zip",     ARCHIVE_FORMAT_ZIP, 0},
    {"7z",      ARCHIVE_FORMAT_7ZIP, 0},
    {NULL, 0, 0}
};

static void print_help(void)
{
    printf(
        "Usage: pack [-v] [-o output] <format> <source> [source...]\n"
        "Packs files or directories into the specified format.\n"
        "Archive member paths are always relative (basename of each source).\n"
        "Success is silent; use -v to list members as they are packed.\n"
        "\n"
        "Supported formats:\n"
        "  tar, tar.gz, tar.xz, tar.bz2, tar.zst, tar.lz4, tar.lz, tar.lzo, tar.br\n"
        "  zip, 7z\n"
        "  gz, bz2, xz, zstd, lz4, lzo, br  (single file only)\n"
        "\n"
        "Options:\n"
        "  -o, --output FILE  Output archive path (default: <basename>.<format>)\n"
        "  -v, --verbose      List archive members as they are packed\n"
        "      --version      Show version information and exit\n"
        "  -h, --help         Show this help message and exit\n"
    );
}

static void print_version(void)
{
    printf(
        "pack (pack-unpack) %s\n"
        "Copyright (C) 2026 Iván Ezequiel Rodriguez\n"
        "License GPLv3+: GNU GPL version 3 or later <https://gnu.org/licenses/gpl.html>.\n"
        "This is free software: you are free to change and redistribute it.\n"
        "There is NO WARRANTY, to the extent permitted by law.\n"
        "\n"
        "Source: %s\n"
        "\n"
        "Escrito por Iván Ezequiel Rodriguez.\n",
        PACK_UNPACK_VERSION,
        PACK_UNPACK_SOURCE_URL
    );
}

static char *xstrdup(const char *s)
{
    char *p;

    if (!s)
        return NULL;
    p = strdup(s);
    if (!p)
        fprintf(stderr, "Error: Memory allocation failed\n");
    return p;
}

static char *xasprintf(const char *fmt, ...)
{
    va_list ap;
    char *out = NULL;
    int n;

    va_start(ap, fmt);
    n = vasprintf(&out, fmt, ap);
    va_end(ap);
    if (n < 0 || !out)
    {
        fprintf(stderr, "Error: Memory allocation failed\n");
        free(out);
        return NULL;
    }
    return out;
}

static void hardlink_map_free(HardlinkMap *map)
{
    HardlinkNode *n;

    if (!map)
        return;
    n = map->head;
    while (n)
    {
        HardlinkNode *next = n->next;
        free(n->pathname);
        free(n);
        n = next;
    }
    map->head = NULL;
}

static const char *hardlink_lookup(HardlinkMap *map, dev_t dev, ino_t ino)
{
    HardlinkNode *n;

    for (n = map->head; n; n = n->next)
    {
        if (n->dev == dev && n->ino == ino)
            return n->pathname;
    }
    return NULL;
}

static int hardlink_remember(HardlinkMap *map, dev_t dev, ino_t ino, const char *pathname)
{
    HardlinkNode *n = malloc(sizeof(*n));

    if (!n)
        return -1;
    n->pathname = xstrdup(pathname);
    if (!n->pathname)
    {
        free(n);
        return -1;
    }
    n->dev = dev;
    n->ino = ino;
    n->next = map->head;
    map->head = n;
    return 0;
}

/* Absolute path for comparison; works when the final component does not exist yet. */
static char *abs_path_dup(const char *path)
{
    char *rp;
    char *dir_copy = NULL;
    char *base_copy = NULL;
    char *dir;
    char *base;
    char *dir_real;
    char *out;

    if (!path)
        return NULL;

    rp = realpath(path, NULL);
    if (rp)
        return rp;

    dir_copy = strdup(path);
    base_copy = strdup(path);
    if (!dir_copy || !base_copy)
    {
        free(dir_copy);
        free(base_copy);
        return NULL;
    }
    dir = dirname(dir_copy);
    base = basename(base_copy);
    dir_real = realpath(dir, NULL);
    if (!dir_real)
    {
        free(dir_copy);
        free(base_copy);
        return NULL;
    }
    out = xasprintf("%s/%s", dir_real, base);
    free(dir_real);
    free(dir_copy);
    free(base_copy);
    return out;
}

static int output_is_source_path(const char *output, char **sources, int nsources)
{
    char *out_abs;
    int i;
    int ret = 0;

    out_abs = abs_path_dup(output);
    if (!out_abs)
    {
        fprintf(stderr, "Error: Cannot resolve output path: %s\n", output);
        return -1;
    }

    for (i = 0; i < nsources; i++)
    {
        char *src_abs = realpath(sources[i], NULL);

        if (!src_abs)
            continue;
        if (strcmp(out_abs, src_abs) == 0)
        {
            fprintf(stderr,
                    "Error: Output '%s' is the same as source '%s'\n",
                    output, sources[i]);
            free(src_abs);
            ret = 1;
            break;
        }
        free(src_abs);
    }
    free(out_abs);
    return ret;
}

/*
 * Basename of path for archive root. "." / ".." are resolved via realpath so
 * members stay relative and safe for unpack.
 * Returns malloc'd string; caller frees. NULL on error.
 */
static char *archive_root_name(const char *path)
{
    char *dup;
    char *base;
    char *rp;
    size_t len;
    char *out;

    if (!path)
        return NULL;

    dup = strdup(path);
    if (!dup)
    {
        fprintf(stderr, "Error: Memory allocation failed\n");
        return NULL;
    }

    len = strlen(dup);
    while (len > 1 && dup[len - 1] == '/')
    {
        dup[len - 1] = '\0';
        len--;
    }

    base = basename(dup);
    if (!base || base[0] == '\0' || strcmp(base, "/") == 0)
    {
        free(dup);
        fprintf(stderr, "Error: Cannot derive archive name from path: %s\n", path);
        return NULL;
    }

    if (strcmp(base, ".") == 0 || strcmp(base, "..") == 0)
    {
        rp = realpath(path, NULL);
        free(dup);
        if (!rp)
        {
            fprintf(stderr, "Error: Cannot resolve path: %s: %s\n",
                    path, strerror(errno));
            return NULL;
        }
        dup = rp;
        len = strlen(dup);
        while (len > 1 && dup[len - 1] == '/')
        {
            dup[len - 1] = '\0';
            len--;
        }
        base = basename(dup);
        if (!base || base[0] == '\0' || strcmp(base, "/") == 0 ||
            strcmp(base, ".") == 0 || strcmp(base, "..") == 0)
        {
            free(dup);
            fprintf(stderr, "Error: Cannot derive archive name from path: %s\n", path);
            return NULL;
        }
    }

    out = xstrdup(base);
    free(dup);
    return out;
}

static int check_unique_archive_roots(char **sources, int nsources)
{
    char **roots;
    int i, j;
    int ret = 0;

    if (nsources < 2)
        return 0;

    roots = calloc((size_t)nsources, sizeof(char *));
    if (!roots)
    {
        fprintf(stderr, "Error: Memory allocation failed\n");
        return -1;
    }

    for (i = 0; i < nsources; i++)
    {
        roots[i] = archive_root_name(sources[i]);
        if (!roots[i])
        {
            ret = -1;
            goto out;
        }
        for (j = 0; j < i; j++)
        {
            if (strcmp(roots[j], roots[i]) == 0)
            {
                fprintf(stderr,
                        "Error: multiple sources map to archive path '%s'\n",
                        roots[i]);
                ret = -1;
                goto out;
            }
        }
    }

out:
    for (i = 0; i < nsources; i++)
        free(roots[i]);
    free(roots);
    return ret;
}

static int write_header_checked(struct archive *a, struct archive_entry *entry)
{
    int r = archive_write_header(a, entry);

    if (r != ARCHIVE_OK && r != ARCHIVE_WARN)
    {
        fprintf(stderr, "Error: Failed to write header: %s\n", archive_error_string(a));
        return -1;
    }
    if (r == ARCHIVE_WARN)
        fprintf(stderr, "Warning: %s\n", archive_error_string(a));
    return 0;
}

/* Open path, fstat that fd, copy size bytes; short read is an error. */
static int write_file_data(struct archive *a, const char *fs_path, la_int64_t expected_size)
{
    int fd;
    struct stat st;
    char buf[8192];
    la_int64_t remaining;
    la_int64_t size;

    fd = open(fs_path, O_RDONLY);
    if (fd < 0)
    {
        fprintf(stderr, "Error: Cannot open %s: %s\n", fs_path, strerror(errno));
        return -1;
    }
    if (fstat(fd, &st) != 0)
    {
        fprintf(stderr, "Error: Cannot fstat %s: %s\n", fs_path, strerror(errno));
        close(fd);
        return -1;
    }
    if (!S_ISREG(st.st_mode))
    {
        fprintf(stderr, "Error: unsupported file type: %s\n", fs_path);
        close(fd);
        return -1;
    }

    size = (la_int64_t)st.st_size;
    if (expected_size >= 0 && size != expected_size)
    {
        /* Prefer the size we already published in the header. */
        size = expected_size;
    }
    remaining = size;

    while (remaining > 0)
    {
        size_t to_read = sizeof(buf);
        ssize_t n;
        ssize_t written;

        if ((la_int64_t)to_read > remaining)
            to_read = (size_t)remaining;

        n = read(fd, buf, to_read);
        if (n < 0)
        {
            fprintf(stderr, "Error: Failed reading %s: %s\n", fs_path, strerror(errno));
            close(fd);
            return -1;
        }
        if (n == 0)
        {
            fprintf(stderr, "Error: Unexpected EOF reading %s\n", fs_path);
            close(fd);
            return -1;
        }

        written = archive_write_data(a, buf, (size_t)n);
        if (written < 0 || (size_t)written != (size_t)n)
        {
            fprintf(stderr, "Error: Failed to write data: %s\n", archive_error_string(a));
            close(fd);
            return -1;
        }
        remaining -= (la_int64_t)n;
    }

    if (close(fd) != 0)
    {
        fprintf(stderr, "Error: Failed closing %s: %s\n", fs_path, strerror(errno));
        return -1;
    }
    return 0;
}

/*
 * Map a disk entry pathname onto the archive root.
 * source_open: path passed to archive_read_disk_open.
 * root: archive basename for that source.
 */
static char *remap_archive_pathname(const char *source_open, const char *root,
                                    const char *entry_path)
{
    size_t slen;
    char *source_real;
    char *entry_real;
    char *out = NULL;

    if (!entry_path || !root)
        return NULL;

    if (strcmp(entry_path, source_open) == 0)
        return xstrdup(root);

    slen = strlen(source_open);
    if (slen > 0 && strncmp(entry_path, source_open, slen) == 0 &&
        (entry_path[slen] == '/' || entry_path[slen] == '\0'))
    {
        if (entry_path[slen] == '\0')
            return xstrdup(root);
        return xasprintf("%s%s", root, entry_path + slen);
    }

    source_real = realpath(source_open, NULL);
    if (source_real)
    {
        slen = strlen(source_real);
        if (strncmp(entry_path, source_real, slen) == 0 &&
            (entry_path[slen] == '/' || entry_path[slen] == '\0'))
        {
            if (entry_path[slen] == '\0')
                out = xstrdup(root);
            else
                out = xasprintf("%s%s", root, entry_path + slen);
            free(source_real);
            return out;
        }

        entry_real = realpath(entry_path, NULL);
        if (entry_real)
        {
            if (strcmp(entry_real, source_real) == 0)
                out = xstrdup(root);
            else if (strncmp(entry_real, source_real, slen) == 0 &&
                     entry_real[slen] == '/')
                out = xasprintf("%s%s", root, entry_real + slen);
            free(entry_real);
        }
        free(source_real);
        if (out)
            return out;
    }

    /* Fallback: single-component entry matches root semantics. */
    if (strchr(entry_path, '/') == NULL)
        return xstrdup(root);

    fprintf(stderr, "Error: Cannot map path into archive: %s\n", entry_path);
    return NULL;
}

static int entry_is_supported(struct archive_entry *entry, const char *fs_path)
{
    mode_t ft = archive_entry_filetype(entry);

    if (ft == AE_IFREG || ft == AE_IFDIR || ft == AE_IFLNK)
        return 1;
    /* Hardlink entries still report as regular with hardlink set. */
    if (archive_entry_hardlink(entry))
        return 1;

    fprintf(stderr, "Error: unsupported file type: %s\n",
            fs_path ? fs_path : archive_entry_pathname(entry));
    return 0;
}

static int should_skip_path(const char *fs_path, int have_skip, dev_t skip_dev,
                            ino_t skip_ino, const char *skip_path_abs)
{
    struct stat st;
    char *cur;

    if (!fs_path)
        return 0;

    if (have_skip && lstat(fs_path, &st) == 0 &&
        st.st_dev == skip_dev && st.st_ino == skip_ino)
        return 1;

    if (skip_path_abs)
    {
        cur = realpath(fs_path, NULL);
        if (cur)
        {
            int same = (strcmp(cur, skip_path_abs) == 0);

            free(cur);
            if (same)
                return 1;
        }
    }
    return 0;
}

static int write_entry_to_archive(struct archive *a, struct archive_entry *entry,
                                  const char *fs_path, const char *archive_path,
                                  HardlinkMap *hl)
{
    const char *prev;
    mode_t ft;
    la_int64_t size;
    int r;

    if (!entry_is_supported(entry, fs_path))
        return -1;

    archive_entry_set_pathname(entry, archive_path);

    ft = archive_entry_filetype(entry);

    if (ft == AE_IFLNK)
    {
        if (g_verbose)
            printf("%s\n", archive_path);
        return write_header_checked(a, entry);
    }

    if (ft == AE_IFDIR)
    {
        if (g_verbose)
            printf("%s\n", archive_path);
        return write_header_checked(a, entry);
    }

    /* Regular file (or hardlink). */
    if (archive_entry_nlink(entry) > 1)
    {
        prev = hardlink_lookup(hl, (dev_t)archive_entry_dev(entry),
                               (ino_t)archive_entry_ino64(entry));
        if (prev)
        {
            archive_entry_set_hardlink(entry, prev);
            archive_entry_set_size(entry, 0);
            if (g_verbose)
                printf("%s\n", archive_path);
            return write_header_checked(a, entry);
        }
        if (hardlink_remember(hl, (dev_t)archive_entry_dev(entry),
                              (ino_t)archive_entry_ino64(entry), archive_path) != 0)
        {
            fprintf(stderr, "Error: Memory allocation failed\n");
            return -1;
        }
    }

    size = archive_entry_size(entry);
    if (g_verbose)
        printf("%s\n", archive_path);
    r = write_header_checked(a, entry);
    if (r != 0)
        return -1;

    if (size > 0 && !archive_entry_hardlink(entry))
    {
        if (write_file_data(a, fs_path, size) != 0)
            return -1;
    }
    return 0;
}

static int add_source_via_read_disk(struct archive *a, const char *source,
                                    const char *root, HardlinkMap *hl,
                                    int have_skip, dev_t skip_dev, ino_t skip_ino,
                                    const char *skip_path_abs)
{
    struct archive *disk = NULL;
    struct archive_entry *entry = NULL;
    int r;
    int status = -1;

    disk = archive_read_disk_new();
    if (!disk)
    {
        fprintf(stderr, "Error: Failed to create disk reader\n");
        return -1;
    }

    archive_read_disk_set_standard_lookup(disk);
    archive_read_disk_set_symlink_physical(disk);

    r = archive_read_disk_open(disk, source);
    if (r != ARCHIVE_OK)
    {
        fprintf(stderr, "Error: Cannot open %s: %s\n", source, archive_error_string(disk));
        goto out;
    }

    for (;;)
    {
        const char *srcpath;
        const char *disk_path;
        char *mapped = NULL;

        entry = archive_entry_new();
        if (!entry)
        {
            fprintf(stderr, "Error: Memory allocation failed\n");
            goto out;
        }

        r = archive_read_next_header2(disk, entry);
        if (r == ARCHIVE_EOF)
        {
            archive_entry_free(entry);
            entry = NULL;
            break;
        }
        if (r != ARCHIVE_OK)
        {
            fprintf(stderr, "Error: Failed reading %s: %s\n",
                    source, archive_error_string(disk));
            goto out;
        }

        if (archive_read_disk_can_descend(disk))
        {
            if (archive_read_disk_descend(disk) != ARCHIVE_OK)
            {
                fprintf(stderr, "Error: Failed descending %s: %s\n",
                        source, archive_error_string(disk));
                goto out;
            }
        }

        srcpath = archive_entry_sourcepath(entry);
        disk_path = srcpath ? srcpath : archive_entry_pathname(entry);
        if (!disk_path)
        {
            fprintf(stderr, "Error: Missing path for archive entry\n");
            goto out;
        }

        if (should_skip_path(disk_path, have_skip, skip_dev, skip_ino, skip_path_abs))
        {
            archive_entry_free(entry);
            entry = NULL;
            continue;
        }

        mapped = remap_archive_pathname(source, root, archive_entry_pathname(entry));
        if (!mapped)
        {
            /* Try with sourcepath when pathname mapping failed. */
            mapped = remap_archive_pathname(source, root, disk_path);
        }
        if (!mapped)
            goto out;

        if (write_entry_to_archive(a, entry, disk_path, mapped, hl) != 0)
        {
            free(mapped);
            goto out;
        }
        free(mapped);
        archive_entry_free(entry);
        entry = NULL;
    }

    status = 0;

out:
    if (entry)
        archive_entry_free(entry);
    if (disk)
    {
        archive_read_close(disk);
        archive_read_free(disk);
    }
    return status;
}

int main(int argc, char *argv[])
{
    const char *format = NULL;
    const char *output_opt = NULL;
    char **sources = NULL;
    int nsources = 0;
    int i;
    const PackFormat *fmt = NULL;
    struct archive *a = NULL;
    HardlinkMap hl = {NULL};
    char *output_heap = NULL;
    const char *output;
    char *temp_path = NULL;
    int temp_fd = -1;
    int have_temp = 0;
    int status = 1;
    char *skip_path_abs = NULL;

    for (i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--version") == 0)
        {
            print_version();
            return 0;
        }
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0)
        {
            print_help();
            return 0;
        }
        if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0)
        {
            g_verbose = 1;
            continue;
        }
        if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output") == 0)
        {
            if (i + 1 >= argc)
            {
                fprintf(stderr, "Error: %s requires an argument\n", argv[i]);
                return 1;
            }
            output_opt = argv[++i];
            continue;
        }
        if (argv[i][0] == '-' && argv[i][1] != '\0')
        {
            fprintf(stderr, "Error: Unknown option: %s\n", argv[i]);
            print_help();
            return 1;
        }
        break;
    }

    if (i >= argc)
    {
        print_help();
        return 1;
    }
    format = argv[i++];
    if (i >= argc)
    {
        print_help();
        return 1;
    }

    sources = &argv[i];
    nsources = argc - i;

    for (fmt = formats; fmt->format; ++fmt)
    {
        if (strcmp(fmt->format, format) == 0)
            break;
    }
    if (!fmt->format)
    {
        fprintf(stderr, "Error: Unsupported format: %s\n", format);
        return 1;
    }

    if (fmt->format_id == ARCHIVE_FORMAT_RAW)
    {
        struct stat st;

        if (nsources != 1)
        {
            fprintf(stderr, "Error: Format '%s' requires exactly one file\n", format);
            return 1;
        }
        if (lstat(sources[0], &st) != 0)
        {
            fprintf(stderr, "Error: Source not found: %s\n", sources[0]);
            return 1;
        }
        if (!S_ISREG(st.st_mode))
        {
            fprintf(stderr, "Error: Format '%s' only supports single regular files\n", format);
            fprintf(stderr, "Use tar.gz, tar.xz, tar.bz2, tar.zst, zip, or 7z for directories\n");
            return 1;
        }
    }

    for (i = 0; i < nsources; i++)
    {
        struct stat st;

        if (lstat(sources[i], &st) != 0)
        {
            fprintf(stderr, "Error: Source not found: %s\n", sources[i]);
            return 1;
        }
    }

    if (output_opt)
    {
        output = output_opt;
    }
    else
    {
        char *root = archive_root_name(sources[0]);

        if (!root)
            return 1;
        output_heap = xasprintf("%s.%s", root, format);
        free(root);
        if (!output_heap)
            return 1;
        output = output_heap;
    }

    {
        int clash = output_is_source_path(output, sources, nsources);

        if (clash != 0)
        {
            free(output_heap);
            return 1;
        }
    }

    if (check_unique_archive_roots(sources, nsources) != 0)
    {
        free(output_heap);
        return 1;
    }

    /* Atomic write: mkstemp keeps the fd; open_fd uses it; rename on success. */
    {
        char *out_dup;
        char *dir;
        mode_t mask;

        out_dup = strdup(output);
        if (!out_dup)
        {
            fprintf(stderr, "Error: Memory allocation failed\n");
            free(output_heap);
            return 1;
        }
        dir = dirname(out_dup);
        temp_path = xasprintf("%s/.#pack-XXXXXX", dir);
        free(out_dup);
        if (!temp_path)
        {
            free(output_heap);
            return 1;
        }

        temp_fd = mkstemp(temp_path);
        if (temp_fd < 0)
        {
            fprintf(stderr, "Error: Cannot create temporary file: %s\n", strerror(errno));
            free(temp_path);
            free(output_heap);
            return 1;
        }
        have_temp = 1;

        mask = umask(0);
        umask(mask);
        if (fchmod(temp_fd, 0666 & ~mask) != 0)
        {
            fprintf(stderr, "Error: Cannot set temporary file mode: %s\n", strerror(errno));
            goto fail;
        }
    }

    a = archive_write_new();
    if (!a)
    {
        fprintf(stderr, "Error: Failed to create archive\n");
        goto fail;
    }

    if (archive_write_set_format(a, fmt->format_id) != ARCHIVE_OK)
    {
        fprintf(stderr, "Error: Failed to set format: %s\n", archive_error_string(a));
        goto fail;
    }
    if (fmt->filter_id)
    {
        if (archive_write_add_filter(a, fmt->filter_id) != ARCHIVE_OK)
        {
            fprintf(stderr, "Error: Failed to add filter: %s\n", archive_error_string(a));
            goto fail;
        }
    }

    if (archive_write_open_fd(a, temp_fd) != ARCHIVE_OK)
    {
        fprintf(stderr, "Error: Failed to open output file: %s\n", archive_error_string(a));
        goto fail;
    }

    {
        struct stat temp_st;
        int have_skip = 0;
        dev_t skip_dev = 0;
        ino_t skip_ino = 0;

        if (fstat(temp_fd, &temp_st) != 0)
        {
            fprintf(stderr, "Error: Cannot fstat temporary: %s\n", strerror(errno));
            goto fail;
        }
        if (archive_write_set_skip_file(a, temp_st.st_dev, temp_st.st_ino) != ARCHIVE_OK)
        {
            fprintf(stderr, "Error: Failed to set skip_file: %s\n", archive_error_string(a));
            goto fail;
        }
        have_skip = 1;
        skip_dev = temp_st.st_dev;
        skip_ino = temp_st.st_ino;

        skip_path_abs = realpath(output, NULL);

        for (i = 0; i < nsources; i++)
        {
            char *root = archive_root_name(sources[i]);

            if (!root)
                goto fail;
            if (add_source_via_read_disk(a, sources[i], root, &hl, have_skip,
                                         skip_dev, skip_ino, skip_path_abs) != 0)
            {
                free(root);
                goto fail;
            }
            free(root);
        }
    }

    if (archive_write_close(a) != ARCHIVE_OK)
    {
        fprintf(stderr, "Error: Failed to close archive: %s\n", archive_error_string(a));
        goto fail;
    }

    if (close(temp_fd) != 0)
    {
        fprintf(stderr, "Error: Failed closing temporary file: %s\n", strerror(errno));
        temp_fd = -1;
        goto fail;
    }
    temp_fd = -1;

    if (rename(temp_path, output) != 0)
    {
        fprintf(stderr, "Error: Cannot rename temporary to %s: %s\n",
                output, strerror(errno));
        goto fail;
    }
    have_temp = 0;
    status = 0;

fail:
    hardlink_map_free(&hl);
    free(skip_path_abs);
    if (a)
        archive_write_free(a);
    if (temp_fd >= 0)
        close(temp_fd);
    if (have_temp && temp_path)
        unlink(temp_path);
    free(temp_path);
    free(output_heap);
    return status;
}
