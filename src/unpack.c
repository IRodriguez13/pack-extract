/**
 * unpack - Unified archive extraction tool (C, libarchive)
 * Copyright (C) 2026 Iván Ezequiel Rodriguez
 * License: GPLv3+
 *
 * Usage: unpack [-v] [-p] [-C dir] [-f|-n|-i] [--] <archive>
 * Alias: extract (optional symlink; avoid when GNU libextractor owns /usr/bin/extract)
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <libgen.h>
#include <unistd.h>
#include <errno.h>
#include <getopt.h>
#include <sys/stat.h>
#include <archive.h>
#include <archive_entry.h>
#include "version.h"

#define EXTRACT_BLOCK_SIZE 10240

typedef enum
{
    OVERWRITE_DEFAULT = 0, /* TTY: ask; non-TTY: refuse */
    OVERWRITE_FORCE,
    OVERWRITE_NOCLOBBER,
    OVERWRITE_INTERACTIVE
} OverwriteMode;

static int g_verbose = 0;
static int g_preserve = 0;

static const char *prog_name(const char *argv0)
{
    const char *base;

    if (!argv0 || argv0[0] == '\0')
        return "unpack";
    base = strrchr(argv0, '/');
    base = base ? base + 1 : argv0;
    if (strcmp(base, "extract") == 0)
        return "extract";
    return "unpack";
}

static void print_help(const char *argv0)
{
    const char *name = prog_name(argv0);

    printf(
        "Usage: %s [-v] [-p] [-C dir] [-f|-n|-i] [--] <archive>\n"
        "Unpacks compressed archives automatically by detecting the format if supported.\n"
        "Canonical command: unpack. Optional alias: extract (symlink).\n"
        "Success is silent; use -v to list members as they are unpacked.\n"
        "\n"
        "Options:\n"
        "  -C, --directory DIR  Change to DIR before unpacking\n"
        "  -f, --force          Overwrite existing files without prompting\n"
        "  -n, --no-clobber     Never overwrite; skip existing paths\n"
        "  -i, --interactive    Always prompt before overwrite (requires a tty)\n"
        "  -p, --preserve       Restore full mode bits, ACLs, and file flags\n"
        "  -v, --verbose        List members as they are unpacked\n"
        "      --version        Show version information and exit\n"
        "  -h, --help           Show this help message and exit\n"
        "\n"
        "Default extraction restores content, structure, links, and mtime under the\n"
        "process umask. Use -p for exhaustive metadata (PERM/ACL/FFLAGS).\n"
        "Default overwrite policy: prompt on a tty; refuse conflicts otherwise.\n"
        "Only one of -f, -n, or -i may be specified.\n",
        name
    );
}

static void print_version(const char *argv0)
{
    printf(
        "%s (pack-unpack) %s\n"
        "Copyright (C) 2026 Iván Ezequiel Rodriguez\n"
        "License GPLv3+: GNU GPL version 3 or later <https://gnu.org/licenses/gpl.html>.\n"
        "This is free software: you are free to change and redistribute it.\n"
        "There is NO WARRANTY, to the extent permitted by law.\n"
        "\n"
        "Source: %s\n"
        "\n"
        "Escrito por Iván Ezequiel Rodriguez.\n",
        prog_name(argv0),
        PACK_UNPACK_VERSION,
        PACK_UNPACK_SOURCE_URL
    );
}

/* Reject absolute paths and ".." components (zip-slip / hardlink targets). */
static int entry_path_is_safe(const char *pathname)
{
    const char *p;

    if (!pathname || pathname[0] == '\0')
        return 0;
    if (pathname[0] == '/')
        return 0;

    for (p = pathname; *p; )
    {
        if (p[0] == '.' && p[1] == '.' && (p[2] == '/' || p[2] == '\0'))
            return 0;
        while (*p && *p != '/')
            p++;
        while (*p == '/')
            p++;
    }
    return 1;
}

static int ask_overwrite(const char *pathname)
{
    char line[64];

    if (!isatty(STDIN_FILENO) || !isatty(STDERR_FILENO))
    {
        fprintf(stderr,
                "Error: '%s' already exists (non-interactive; refusing overwrite)\n",
                pathname);
        return -1;
    }

    for (;;)
    {
        fprintf(stderr, "File exists: %s\nOverwrite / replace? [y/N] ", pathname);
        fflush(stderr);

        if (!fgets(line, sizeof(line), stdin))
        {
            fprintf(stderr, "\nCancelled.\n");
            return -1;
        }

        char *p = line;
        while (*p && isspace((unsigned char)*p))
            p++;
        if (*p == '\0')
            return 0;

        char c = (char)tolower((unsigned char)*p);
        if (c == 'y')
            return 1;
        if (c == 'n')
            return 0;

        fprintf(stderr, "Please answer y or n (Ctrl+C to cancel).\n");
    }
}

static int is_existing_dir_merge(const char *pathname, struct archive_entry *entry)
{
    struct stat st;

    if (lstat(pathname, &st) != 0)
        return 0;
    if (!S_ISDIR(st.st_mode))
        return 0;
    return archive_entry_filetype(entry) == AE_IFDIR;
}

static int path_exists(const char *pathname)
{
    struct stat st;
    return lstat(pathname, &st) == 0;
}

static int resolve_overwrite(OverwriteMode mode, const char *pathname)
{
    switch (mode)
    {
    case OVERWRITE_FORCE:
        return 1;
    case OVERWRITE_NOCLOBBER:
        return 0;
    case OVERWRITE_INTERACTIVE:
    case OVERWRITE_DEFAULT:
    default:
        return ask_overwrite(pathname);
    }
}

static int is_raw_archive_format(struct archive *a)
{
    int fmt = archive_format(a);

#ifdef ARCHIVE_FORMAT_BASE_MASK
    return (fmt & ARCHIVE_FORMAT_BASE_MASK) == ARCHIVE_FORMAT_RAW;
#else
    return fmt == ARCHIVE_FORMAT_RAW;
#endif
}

/* True if libarchive already bid a real archive container (not mtree noise). */
static int is_solid_container_format(struct archive *a)
{
    int fmt = archive_format(a);
    int base;

#ifdef ARCHIVE_FORMAT_BASE_MASK
    base = fmt & ARCHIVE_FORMAT_BASE_MASK;
#else
    base = fmt;
#endif

    switch (base)
    {
    case ARCHIVE_FORMAT_TAR:
    case ARCHIVE_FORMAT_CPIO:
    case ARCHIVE_FORMAT_ZIP:
    case ARCHIVE_FORMAT_7ZIP:
#ifdef ARCHIVE_FORMAT_ISO9660
    case ARCHIVE_FORMAT_ISO9660:
#endif
#ifdef ARCHIVE_FORMAT_XAR
    case ARCHIVE_FORMAT_XAR:
#endif
#ifdef ARCHIVE_FORMAT_AR
    case ARCHIVE_FORMAT_AR:
#endif
#ifdef ARCHIVE_FORMAT_WARC
    case ARCHIVE_FORMAT_WARC:
#endif
#ifdef ARCHIVE_FORMAT_RAR
    case ARCHIVE_FORMAT_RAR:
#endif
#ifdef ARCHIVE_FORMAT_RAR_V5
    case ARCHIVE_FORMAT_RAR_V5:
#endif
#ifdef ARCHIVE_FORMAT_CAB
    case ARCHIVE_FORMAT_CAB:
#endif
#ifdef ARCHIVE_FORMAT_LHA
    case ARCHIVE_FORMAT_LHA:
#endif
        return 1;
    default:
        return 0;
    }
}

/*
 * RAW fallback when no solid container was identified.
 * Corrupt tar.gz (format already TAR) must not become a RAW gunzip.
 * Bare .gz may falsely bid mtree under format_all — that still retries RAW.
 */
static int should_retry_as_raw(struct archive *a)
{
    return !is_solid_container_format(a);
}

/* Returns malloc d name; caller frees. */
static char *derive_raw_member_name(const char *archive_path)
{
    static const char *const suffixes[] = {
        ".gz", ".bz2", ".xz", ".zst", ".zstd", ".lz4", ".lzo", ".lz", ".br",
        NULL
    };
    char *dup;
    char *base;
    size_t blen;
    const char *const *s;
    char *out;

    if (!archive_path)
        return NULL;

    dup = strdup(archive_path);
    if (!dup)
        return NULL;
    base = basename(dup);
    blen = strlen(base);

    for (s = suffixes; *s; s++)
    {
        size_t slen = strlen(*s);

        if (blen > slen && strcmp(base + blen - slen, *s) == 0)
        {
            size_t keep = blen - slen;

            if (keep == 0)
            {
                free(dup);
                return NULL;
            }
            out = malloc(keep + 1);
            if (!out)
            {
                free(dup);
                return NULL;
            }
            memcpy(out, base, keep);
            out[keep] = '\0';
            free(dup);
            return out;
        }
    }

    out = strdup(base);
    free(dup);
    return out;
}

static struct archive *open_archive_reader(const char *filename, int raw_only)
{
    struct archive *a = archive_read_new();

    if (!a)
        return NULL;

    archive_read_support_filter_all(a);
    if (raw_only)
        archive_read_support_format_raw(a);
    else
        archive_read_support_format_all(a);

    if (archive_read_open_filename(a, filename, EXTRACT_BLOCK_SIZE) != ARCHIVE_OK)
    {
        archive_read_free(a);
        return NULL;
    }
    return a;
}

static int skip_entry_data(struct archive *a, const char *pathname)
{
    if (g_verbose)
        printf("Skipping: %s\n", pathname);
    if (archive_read_data_skip(a) != ARCHIVE_OK)
    {
        fprintf(stderr, "Error: Failed to skip entry: %s\n", archive_error_string(a));
        return -1;
    }
    return 0;
}

static int unpack_archive(const char *filename, OverwriteMode mode)
{
    struct archive *a = NULL;
    struct archive *ext = NULL;
    struct archive_entry *entry;
    int r;
    int status = 1;
    int disk_flags;
    int closed_ext = 0;
    char *raw_name = NULL;

    a = open_archive_reader(filename, 0);
    if (!a)
    {
        fprintf(stderr, "Error: Failed to open archive: %s\n", filename);
        return 1;
    }

    r = archive_read_next_header(a, &entry);
    if (r != ARCHIVE_OK)
    {
        if (!should_retry_as_raw(a))
        {
            fprintf(stderr, "Error: Failed to read archive: %s\n",
                    archive_error_string(a));
            goto fail;
        }
        archive_read_free(a);
        a = open_archive_reader(filename, 1);
        if (!a)
        {
            fprintf(stderr, "Error: Failed to open archive: %s\n", filename);
            return 1;
        }
        r = archive_read_next_header(a, &entry);
        if (r != ARCHIVE_OK)
        {
            fprintf(stderr, "Error: Failed to read archive: %s\n",
                    archive_error_string(a));
            goto fail;
        }
    }

    ext = archive_write_disk_new();
    if (!ext)
    {
        fprintf(stderr, "Error: Failed to create disk writer\n");
        goto fail;
    }

    /*
     * Default: content + structure + links + mtime + security + atomic writes.
     * Mode bits follow the process umask unless -p/--preserve.
     */
    disk_flags = ARCHIVE_EXTRACT_TIME |
                 ARCHIVE_EXTRACT_SECURE_SYMLINKS |
                 ARCHIVE_EXTRACT_SECURE_NODOTDOT |
                 ARCHIVE_EXTRACT_SECURE_NOABSOLUTEPATHS |
                 ARCHIVE_EXTRACT_SAFE_WRITES;
    if (g_preserve)
        disk_flags |= ARCHIVE_EXTRACT_PERM | ARCHIVE_EXTRACT_ACL |
                      ARCHIVE_EXTRACT_FFLAGS;
    if (mode == OVERWRITE_FORCE)
        disk_flags |= ARCHIVE_EXTRACT_UNLINK;
    else
        disk_flags |= ARCHIVE_EXTRACT_NO_OVERWRITE;

    archive_write_disk_set_options(ext, disk_flags);
    archive_write_disk_set_standard_lookup(ext);

    do
    {
        const char *pathname = archive_entry_pathname(entry);
        const char *hardlink;
        const void *buff;
        size_t size;
        la_int64_t offset;

        free(raw_name);
        raw_name = NULL;

        if (is_raw_archive_format(a))
        {
            raw_name = derive_raw_member_name(filename);
            if (!raw_name)
            {
                fprintf(stderr, "Error: Cannot derive output name for raw stream\n");
                goto fail;
            }
            archive_entry_set_pathname(entry, raw_name);
            pathname = raw_name;
        }

        if (!entry_path_is_safe(pathname))
        {
            fprintf(stderr, "Error: Refusing unsafe path in archive: %s\n",
                    pathname ? pathname : "(null)");
            goto fail;
        }

        hardlink = archive_entry_hardlink(entry);
        if (hardlink && !entry_path_is_safe(hardlink))
        {
            fprintf(stderr, "Error: Refusing unsafe hardlink target in archive: %s\n",
                    hardlink);
            goto fail;
        }

        if (path_exists(pathname) && !is_existing_dir_merge(pathname, entry))
        {
            int overwrite = resolve_overwrite(mode, pathname);

            if (overwrite < 0)
                goto fail;
            if (overwrite == 0)
            {
                if (skip_entry_data(a, pathname) != 0)
                    goto fail;
                continue;
            }
            if (mode != OVERWRITE_FORCE)
            {
                if (unlink(pathname) != 0 && errno != ENOENT)
                {
                    fprintf(stderr, "Error: Cannot remove '%s': %s\n",
                            pathname, strerror(errno));
                    goto fail;
                }
            }
        }

        if (g_verbose)
            printf("%s\n", pathname);

        r = archive_write_header(ext, entry);
        if (r != ARCHIVE_OK)
        {
            if (mode == OVERWRITE_NOCLOBBER && path_exists(pathname))
            {
                if (skip_entry_data(a, pathname) != 0)
                    goto fail;
                continue;
            }
            fprintf(stderr, "Error: Failed to write header: %s\n", archive_error_string(ext));
            goto fail;
        }

        while ((r = archive_read_data_block(a, &buff, &size, &offset)) == ARCHIVE_OK)
        {
            if (archive_write_data_block(ext, buff, size, offset) != ARCHIVE_OK)
            {
                fprintf(stderr, "Error: Failed to write data: %s\n", archive_error_string(ext));
                goto fail;
            }
        }
        if (r != ARCHIVE_EOF)
        {
            fprintf(stderr, "Error: Failed to read data: %s\n", archive_error_string(a));
            goto fail;
        }

        if (archive_write_finish_entry(ext) != ARCHIVE_OK)
        {
            fprintf(stderr, "Error: Failed to finish entry: %s\n", archive_error_string(ext));
            goto fail;
        }
    } while ((r = archive_read_next_header(a, &entry)) == ARCHIVE_OK);

    if (r != ARCHIVE_EOF)
    {
        fprintf(stderr, "Error: Failed to read archive: %s\n", archive_error_string(a));
        goto fail;
    }

    /* Deferred dir modes / finalization — must check before free. */
    if (archive_write_close(ext) != ARCHIVE_OK)
    {
        fprintf(stderr, "Error: Failed to finalize extraction: %s\n",
                archive_error_string(ext));
        closed_ext = 1;
        goto fail;
    }
    closed_ext = 1;
    status = 0;

fail:
    free(raw_name);
    if (ext)
    {
        if (!closed_ext)
            (void)archive_write_close(ext);
        archive_write_free(ext);
        ext = NULL;
    }
    if (a)
    {
        archive_read_free(a);
        a = NULL;
    }
    return status;
}

int main(int argc, char *argv[])
{
    const char *filename = NULL;
    const char *chdir_to = NULL;
    const char *argv0 = argc > 0 ? argv[0] : "unpack";
    OverwriteMode mode = OVERWRITE_DEFAULT;
    int mode_set = 0;
    int c;
    struct stat st;
    char *archive_abs = NULL;
    int status;

    static const struct option longopts[] = {
        {"directory", required_argument, NULL, 'C'},
        {"force", no_argument, NULL, 'f'},
        {"no-clobber", no_argument, NULL, 'n'},
        {"interactive", no_argument, NULL, 'i'},
        {"preserve", no_argument, NULL, 'p'},
        {"verbose", no_argument, NULL, 'v'},
        {"version", no_argument, NULL, 1},
        {"help", no_argument, NULL, 'h'},
        {NULL, 0, NULL, 0}
    };

    while ((c = getopt_long(argc, argv, "C:fniphv", longopts, NULL)) != -1)
    {
        switch (c)
        {
        case 1:
            print_version(argv0);
            return 0;
        case 'h':
            print_help(argv0);
            return 0;
        case 'v':
            g_verbose = 1;
            break;
        case 'p':
            g_preserve = 1;
            break;
        case 'C':
            chdir_to = optarg;
            break;
        case 'f':
            if (mode_set)
            {
                fprintf(stderr, "Error: Only one of -f, -n, or -i may be specified\n");
                return 1;
            }
            mode = OVERWRITE_FORCE;
            mode_set = 1;
            break;
        case 'n':
            if (mode_set)
            {
                fprintf(stderr, "Error: Only one of -f, -n, or -i may be specified\n");
                return 1;
            }
            mode = OVERWRITE_NOCLOBBER;
            mode_set = 1;
            break;
        case 'i':
            if (mode_set)
            {
                fprintf(stderr, "Error: Only one of -f, -n, or -i may be specified\n");
                return 1;
            }
            mode = OVERWRITE_INTERACTIVE;
            mode_set = 1;
            break;
        default:
            print_help(argv0);
            return 1;
        }
    }

    if (optind >= argc)
    {
        print_help(argv0);
        return 1;
    }
    if (optind + 1 < argc)
    {
        fprintf(stderr, "Error: Unexpected argument: %s\n", argv[optind + 1]);
        print_help(argv0);
        return 1;
    }
    filename = argv[optind];

    if (stat(filename, &st) != 0 || !S_ISREG(st.st_mode))
    {
        fprintf(stderr, "Error: File not found: %s\n", filename);
        return 1;
    }

    archive_abs = realpath(filename, NULL);
    if (!archive_abs)
    {
        fprintf(stderr, "Error: Cannot resolve archive path %s: %s\n",
                filename, strerror(errno));
        return 1;
    }

    if (chdir_to)
    {
        if (chdir(chdir_to) != 0)
        {
            fprintf(stderr, "Error: Cannot change directory to %s: %s\n",
                    chdir_to, strerror(errno));
            free(archive_abs);
            return 1;
        }
    }

    status = unpack_archive(archive_abs, mode);
    free(archive_abs);
    return status;
}
