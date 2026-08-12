/**
 * pack - Unified compression tool rewrite in C
 * Copyright (C) 2026 Iván Ezequiel Rodriguez
 * License: GPLv3+
 *
 * Usage: pack [-o output] <format> <source> [source...]
 *   format: tar, tar.gz, tar.xz, tar.bz2, tar.zst, zip, 7z, ...
 *   source: file or directory to pack (archive pathnames are always relative)
 *   -o:    output filename (optional; default basename(first).format)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <libgen.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <archive.h>
#include <archive_entry.h>
#include "version.h"

typedef struct
{
    const char *format;
    int format_id; /* ARCHIVE FORMAT */
    int filter_id; /* FILTER */
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

/* Supported formats */
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
    /* Single-file streams: RAW container + compression filter (EMPTY has no writer). */
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
        "Usage: pack [-o output] <format> <source> [source...]\n"
        "Packs files or directories into the specified format.\n"
        "Archive member paths are always relative (basename of each source).\n"
        "\n"
        "Supported formats:\n"
        "  tar, tar.gz, tar.xz, tar.bz2, tar.zst, tar.lz4, tar.lz, tar.lzo, tar.br\n"
        "  zip, 7z\n"
        "  gz, bz2, xz, zstd, lz4, lzo, br  (single file only)\n"
        "\n"
        "Options:\n"
        "  -o, --output FILE  Output archive path (default: <basename>.<format>)\n"
        "  -v, --version      Show version information and exit\n"
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

/* Returns existing archive pathname for (dev,ino), or NULL if first sighting. */
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
    n->pathname = strdup(pathname);
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

/*
 * Absolute path for comparison. Works when the final component does not exist
 * yet (resolves the parent directory).
 */
static int abs_path(const char *path, char *out, size_t outsz)
{
    char *rp;
    char *dir_copy = NULL;
    char *base_copy = NULL;
    char *dir;
    char *base;
    char dir_real[PATH_MAX];
    int n;

    if (!path || !out || outsz == 0)
        return -1;

    rp = realpath(path, NULL);
    if (rp)
    {
        if (strlen(rp) >= outsz)
        {
            free(rp);
            return -1;
        }
        memcpy(out, rp, strlen(rp) + 1);
        free(rp);
        return 0;
    }

    dir_copy = strdup(path);
    base_copy = strdup(path);
    if (!dir_copy || !base_copy)
    {
        free(dir_copy);
        free(base_copy);
        return -1;
    }
    dir = dirname(dir_copy);
    base = basename(base_copy);
    if (!realpath(dir, dir_real))
    {
        free(dir_copy);
        free(base_copy);
        return -1;
    }
    n = snprintf(out, outsz, "%s/%s", dir_real, base);
    free(dir_copy);
    free(base_copy);
    if (n < 0 || (size_t)n >= outsz)
        return -1;
    return 0;
}

/* Refuse opening output when it would truncate a source path. */
static int output_is_source_path(const char *output, char **sources, int nsources)
{
    char out_abs[PATH_MAX];
    int i;

    if (abs_path(output, out_abs, sizeof(out_abs)) != 0)
    {
        fprintf(stderr, "Error: Cannot resolve output path: %s\n", output);
        return -1;
    }

    for (i = 0; i < nsources; i++)
    {
        char src_abs[PATH_MAX];

        if (!realpath(sources[i], src_abs))
            continue;
        if (strcmp(out_abs, src_abs) == 0)
        {
            fprintf(stderr,
                    "Error: Output '%s' is the same as source '%s'\n",
                    output, sources[i]);
            return 1;
        }
    }
    return 0;
}

/* basename of path without mutating caller's buffer; strips trailing slashes. */
static int archive_root_name(const char *path, char *out, size_t outsz)
{
    char *dup;
    char *base;
    size_t len;

    if (!path || !out || outsz == 0)
        return -1;

    dup = strdup(path);
    if (!dup)
        return -1;

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
        return -1;
    }
    if (strlen(base) >= outsz)
    {
        free(dup);
        fprintf(stderr, "Error: Path too long: %s\n", path);
        return -1;
    }
    memcpy(out, base, strlen(base) + 1);
    free(dup);
    return 0;
}

/*
 * Reject multi-source packs whose basenames collide inside the archive
 * (e.g. a/config and b/config both map to member "config").
 */
static int check_unique_archive_roots(char **sources, int nsources)
{
    char (*roots)[PATH_MAX];
    int i, j;

    if (nsources < 2)
        return 0;

    roots = calloc((size_t)nsources, PATH_MAX);
    if (!roots)
    {
        fprintf(stderr, "Error: Memory allocation failed\n");
        return -1;
    }

    for (i = 0; i < nsources; i++)
    {
        if (archive_root_name(sources[i], roots[i], PATH_MAX) != 0)
        {
            free(roots);
            return -1;
        }
        for (j = 0; j < i; j++)
        {
            if (strcmp(roots[j], roots[i]) == 0)
            {
                fprintf(stderr,
                        "Error: multiple sources map to archive path '%s'\n",
                        roots[i]);
                free(roots);
                return -1;
            }
        }
    }
    free(roots);
    return 0;
}

/* Returns 0 = ok, -1 = error. */
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

static int write_file_data(struct archive *a, const char *fs_path, la_int64_t size)
{
    FILE *fp;
    char buf[8192];
    size_t len;
    la_int64_t remaining = size;

    fp = fopen(fs_path, "rb");
    if (!fp)
    {
        fprintf(stderr, "Error: Cannot open %s: %s\n", fs_path, strerror(errno));
        return -1;
    }

    while (remaining > 0)
    {
        size_t to_read = sizeof(buf);
        ssize_t written;

        if ((la_int64_t)to_read > remaining)
            to_read = (size_t)remaining;

        len = fread(buf, 1, to_read, fp);
        if (len == 0)
        {
            if (ferror(fp))
            {
                fprintf(stderr, "Error: Failed reading %s\n", fs_path);
                fclose(fp);
                return -1;
            }
            break;
        }

        written = archive_write_data(a, buf, len);
        if (written < 0 || (size_t)written != len)
        {
            fprintf(stderr, "Error: Failed to write data: %s\n", archive_error_string(a));
            fclose(fp);
            return -1;
        }
        remaining -= (la_int64_t)len;
    }

    fclose(fp);
    return 0;
}

static int add_to_archive(struct archive *a, const char *fs_path,
                          const char *archive_path, HardlinkMap *hl,
                          int have_skip, dev_t skip_dev, ino_t skip_ino,
                          const char *skip_path_abs)
{
    struct stat st;
    struct archive_entry *entry;
    const char *prev;
    int r;

    if (lstat(fs_path, &st) != 0)
    {
        fprintf(stderr, "Error: Cannot stat %s: %s\n", fs_path, strerror(errno));
        return -1;
    }

    /* Skip temp inode (do not call write_header — that wedges libarchive). */
    if (have_skip && st.st_dev == skip_dev && st.st_ino == skip_ino)
        return 0;

    /*
     * Skip the final output path by realpath (not inode): a hardlink from
     * output → a source file must still be packed; only the destination name
     * inside a walked tree is excluded (self-archive nesting when replacing).
     */
    if (skip_path_abs)
    {
        char cur_abs[PATH_MAX];

        if (realpath(fs_path, cur_abs) && strcmp(cur_abs, skip_path_abs) == 0)
            return 0;
    }

    entry = archive_entry_new();
    if (!entry)
    {
        fprintf(stderr, "Error: Memory allocation failed\n");
        return -1;
    }

    archive_entry_set_pathname(entry, archive_path);
    archive_entry_copy_stat(entry, &st);

    if (S_ISLNK(st.st_mode))
    {
        char target[PATH_MAX];
        ssize_t n;

        n = readlink(fs_path, target, sizeof(target) - 1);
        if (n < 0)
        {
            fprintf(stderr, "Error: Cannot readlink %s: %s\n", fs_path, strerror(errno));
            archive_entry_free(entry);
            return -1;
        }
        target[n] = '\0';
        archive_entry_set_filetype(entry, AE_IFLNK);
        archive_entry_set_symlink(entry, target);
        archive_entry_set_size(entry, 0);

        r = write_header_checked(a, entry);
        archive_entry_free(entry);
        return r;
    }

    if (S_ISDIR(st.st_mode))
    {
        DIR *dir;
        struct dirent *ent;

        archive_entry_set_filetype(entry, AE_IFDIR);
        r = write_header_checked(a, entry);
        archive_entry_free(entry);
        if (r != 0)
            return -1;

        dir = opendir(fs_path);
        if (!dir)
        {
            fprintf(stderr, "Error: Cannot open directory %s: %s\n",
                    fs_path, strerror(errno));
            return -1;
        }

        while ((ent = readdir(dir)))
        {
            char subpath[PATH_MAX];
            char subbase[PATH_MAX];
            int n;

            if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
                continue;

            n = snprintf(subpath, sizeof(subpath), "%s/%s", fs_path, ent->d_name);
            if (n < 0 || (size_t)n >= sizeof(subpath))
            {
                fprintf(stderr, "Error: Path too long under %s\n", fs_path);
                closedir(dir);
                return -1;
            }
            n = snprintf(subbase, sizeof(subbase), "%s/%s", archive_path, ent->d_name);
            if (n < 0 || (size_t)n >= sizeof(subbase))
            {
                fprintf(stderr, "Error: Archive path too long under %s\n", archive_path);
                closedir(dir);
                return -1;
            }
            if (add_to_archive(a, subpath, subbase, hl, have_skip, skip_dev, skip_ino,
                               skip_path_abs) != 0)
            {
                closedir(dir);
                return -1;
            }
        }
        closedir(dir);
        return 0;
    }

    if (!S_ISREG(st.st_mode))
    {
        archive_entry_free(entry);
        return 0;
    }

    if (st.st_nlink > 1)
    {
        prev = hardlink_lookup(hl, st.st_dev, st.st_ino);
        if (prev)
        {
            archive_entry_set_filetype(entry, AE_IFREG);
            archive_entry_set_hardlink(entry, prev);
            archive_entry_set_size(entry, 0);
            r = write_header_checked(a, entry);
            archive_entry_free(entry);
            return r;
        }
        if (hardlink_remember(hl, st.st_dev, st.st_ino, archive_path) != 0)
        {
            fprintf(stderr, "Error: Memory allocation failed\n");
            archive_entry_free(entry);
            return -1;
        }
    }

    archive_entry_set_filetype(entry, AE_IFREG);
    archive_entry_set_size(entry, st.st_size);
    r = write_header_checked(a, entry);
    if (r != 0)
    {
        archive_entry_free(entry);
        return -1;
    }

    if (st.st_size > 0)
    {
        if (write_file_data(a, fs_path, st.st_size) != 0)
        {
            archive_entry_free(entry);
            return -1;
        }
    }

    archive_entry_free(entry);
    return 0;
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
    char output_buf[PATH_MAX];
    const char *output;
    char temp_path[PATH_MAX];
    int temp_fd = -1;
    int have_temp = 0;
    int status = 1;

    temp_path[0] = '\0';

    for (i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--version") == 0 || strcmp(argv[i], "-v") == 0)
        {
            print_version();
            return 0;
        }
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0)
        {
            print_help();
            return 0;
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
        char root[PATH_MAX];

        if (archive_root_name(sources[0], root, sizeof(root)) != 0)
            return 1;
        if (snprintf(output_buf, sizeof(output_buf), "%s.%s", root, format) >= (int)sizeof(output_buf))
        {
            fprintf(stderr, "Error: Output path too long\n");
            return 1;
        }
        output = output_buf;
    }

    {
        int clash = output_is_source_path(output, sources, nsources);

        if (clash != 0)
            return 1;
    }

    if (check_unique_archive_roots(sources, nsources) != 0)
        return 1;

    /* Write to a temp file in the same directory; rename only on success. */
    {
        char *out_dup;
        char *dir;
        int n;

        out_dup = strdup(output);
        if (!out_dup)
        {
            fprintf(stderr, "Error: Memory allocation failed\n");
            return 1;
        }
        dir = dirname(out_dup);
        n = snprintf(temp_path, sizeof(temp_path), "%s/.#pack-XXXXXX", dir);
        free(out_dup);
        if (n < 0 || (size_t)n >= sizeof(temp_path))
        {
            fprintf(stderr, "Error: Temporary path too long\n");
            return 1;
        }
        temp_fd = mkstemp(temp_path);
        if (temp_fd < 0)
        {
            fprintf(stderr, "Error: Cannot create temporary file: %s\n", strerror(errno));
            return 1;
        }
        close(temp_fd);
        temp_fd = -1;
        have_temp = 1;
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

    if (archive_write_open_filename(a, temp_path) != ARCHIVE_OK)
    {
        fprintf(stderr, "Error: Failed to open output file: %s\n", archive_error_string(a));
        goto fail;
    }

    /* Skip temp inode; also skip final output path by name when replacing in-tree. */
    {
        struct stat temp_st;
        char out_abs[PATH_MAX];
        const char *skip_path_abs = NULL;
        int have_skip = 0;
        dev_t skip_dev = 0;
        ino_t skip_ino = 0;

        if (stat(temp_path, &temp_st) != 0)
        {
            fprintf(stderr, "Error: Cannot stat temporary %s: %s\n",
                    temp_path, strerror(errno));
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

        if (realpath(output, out_abs))
            skip_path_abs = out_abs;

        for (i = 0; i < nsources; i++)
        {
            char root[PATH_MAX];

            if (archive_root_name(sources[i], root, sizeof(root)) != 0)
                goto fail;
            if (add_to_archive(a, sources[i], root, &hl, have_skip, skip_dev, skip_ino,
                               skip_path_abs) != 0)
                goto fail;
        }
    }

    if (archive_write_close(a) != ARCHIVE_OK)
    {
        fprintf(stderr, "Error: Failed to close archive: %s\n", archive_error_string(a));
        goto fail;
    }

    if (rename(temp_path, output) != 0)
    {
        fprintf(stderr, "Error: Cannot rename temporary to %s: %s\n",
                output, strerror(errno));
        goto fail;
    }
    have_temp = 0;

    printf("Archive created successfully: %s\n", output);
    status = 0;

fail:
    hardlink_map_free(&hl);
    if (a)
        archive_write_free(a);
    if (have_temp && temp_path[0] != '\0')
        unlink(temp_path);
    return status;
}
