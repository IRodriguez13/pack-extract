/**
 * pack - Unified compression tool rewrite in C
 * Copyright (C) 2026 Iván Ezequiel Rodriguez
 * License: GPLv3+
 * 
 * Usage: pack <format> <source> [output]
 *   format: tar, tar.gz, tar.xz, tar.bz2, tar.zst, zip, 7z
 *   source: file or directory to pack
 *   output: output filename (optional, auto-generated if omitted)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <sys/stat.h>
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

// Formatos soportados
static const PackFormat formats[] = {
    // Tar con diferentes compresiones
    {"tar",      ARCHIVE_FORMAT_TAR, 0},
    {"tar.gz",   ARCHIVE_FORMAT_TAR, ARCHIVE_FILTER_GZIP},
    {"tar.bz2",  ARCHIVE_FORMAT_TAR, ARCHIVE_FILTER_BZIP2},
    {"tar.xz",   ARCHIVE_FORMAT_TAR, ARCHIVE_FILTER_XZ},
    {"tar.zst",  ARCHIVE_FORMAT_TAR, ARCHIVE_FILTER_ZSTD},
    {"tar.lz4",  ARCHIVE_FORMAT_TAR, ARCHIVE_FILTER_LZ4},
    {"tar.lz",   ARCHIVE_FORMAT_TAR, ARCHIVE_FILTER_LZIP},
    {"tar.lzo",  ARCHIVE_FORMAT_TAR, ARCHIVE_FILTER_LZOP},
#if defined(ARCHIVE_FILTER_BROTLI)
    {"tar.br",   ARCHIVE_FORMAT_TAR, ARCHIVE_FILTER_BROTLI},
#endif
    
    // Compresión simple (single file)
    {"gz",       ARCHIVE_FORMAT_EMPTY, ARCHIVE_FILTER_GZIP},
    {"bz2",      ARCHIVE_FORMAT_EMPTY, ARCHIVE_FILTER_BZIP2},
    {"xz",       ARCHIVE_FORMAT_EMPTY, ARCHIVE_FILTER_XZ},
    {"zstd",     ARCHIVE_FORMAT_EMPTY, ARCHIVE_FILTER_ZSTD},
    {"lz4",      ARCHIVE_FORMAT_EMPTY, ARCHIVE_FILTER_LZ4},
    {"lzo",      ARCHIVE_FORMAT_EMPTY, ARCHIVE_FILTER_LZOP},
#if defined(ARCHIVE_FILTER_BROTLI)
    {"br",       ARCHIVE_FORMAT_EMPTY, ARCHIVE_FILTER_BROTLI},
#endif
    
    // Formatos de archivo
    {"zip",      ARCHIVE_FORMAT_ZIP,   0},
    {"7z",       ARCHIVE_FORMAT_7ZIP,  0},
    {NULL, 0, 0}
};

void print_help(void) {
    printf(
        "Usage: pack <format> <source>\n"
        "Packs files or directories into the specified format\n"
        "\n"
        "Supported formats:\n"
        "  tar, tar.gz, tar.xz, tar.bz2, tar.zst, tar.lz4, tar.lz, tar.lzo, tar.br\n"
        "  zip, 7z\n"
        "  gz, bz2, xz, zstd, lz4, lzo, br  (single file only)\n"
        "\n"
        "Options:\n"
        "  -v, --version    Show version information and exit\n"
        "  -h, --help       Show this help message and exit\n"
    );
}

void print_version(void) {
    printf(
        "pack (pack-extract) %s\n"
        "Copyright (C) 2026 Iván Ezequiel Rodriguez\n"
        "License GPLv3+: GNU GPL version 3 or later <https://gnu.org/licenses/gpl.html>.\n"
        "This is free software: you are free to change and redistribute it.\n"
        "There is NO WARRANTY, to the extent permitted by law.\n"
        "\n"
        "Source: %s\n"
        "\n"
        "Escrito por Iván Ezequiel Rodriguez.\n",
        PACK_EXTRACT_VERSION,
        PACK_EXTRACT_SOURCE_URL
    );
}

// Function to add files to archive
int add_to_archive(struct archive *a, const char *path, const char *base_path) {
    struct stat st;
    if (stat(path, &st) != 0) {
        return -1;
    }

    struct archive_entry *entry = archive_entry_new();
    if (!entry) {
        return -1;
    }

    archive_entry_set_pathname(entry, base_path ? base_path : path);
    archive_entry_copy_stat(entry, &st);

    if (S_ISDIR(st.st_mode)) {
        archive_entry_set_filetype(entry, AE_IFDIR);
        archive_write_header(a, entry);
        archive_entry_free(entry);

        DIR *dir = opendir(path);
        if (!dir) {
            return -1;
        }

        struct dirent *ent;
        while ((ent = readdir(dir))) {
            if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) {
                continue;
            }
            char subpath[PATH_MAX];
            snprintf(subpath, sizeof(subpath), "%s/%s", path, ent->d_name);
            char subbase[PATH_MAX];
            if (base_path) {
                snprintf(subbase, sizeof(subbase), "%s/%s", base_path, ent->d_name);
            } else {
                strcpy(subbase, ent->d_name);
            }
            if (add_to_archive(a, subpath, subbase) != 0) {
                closedir(dir);
                return -1;
            }
        }
        closedir(dir);
    } else if (S_ISREG(st.st_mode)) {
        archive_entry_set_filetype(entry, AE_IFREG);
        archive_write_header(a, entry);

        FILE *fp = fopen(path, "rb");
        if (!fp) {
            archive_entry_free(entry);
            return -1;
        }

        char buf[8192];
        size_t len;
        while ((len = fread(buf, 1, sizeof(buf), fp)) > 0) {
            archive_write_data(a, buf, len);
        }
        fclose(fp);
        archive_entry_free(entry);
    } else {
        // Skip other types
        archive_entry_free(entry);
    }

    return 0;
}

int main(int argc, char *argv[])
{
    char *format = NULL;
    char *source = NULL;

    // Parse arguments
    if (argc == 2) {
        if (strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-v") == 0) {
            print_version();
            return 0;
        } else if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
            print_help();
            return 0;
        } else {
            print_help();
            return 1;
        }
    } else if (argc == 3) {
        format = argv[1];
        source = argv[2];
    } else {
        print_help();
        return 1;
    }

    // Check if source exists
    struct stat st;
    if (stat(source, &st) != 0) {
        fprintf(stderr, "Error: Source not found: %s\n", source);
        return 1;
    }

    // Find format
    const PackFormat *fmt = NULL;
    for (const PackFormat *f = formats; f->format; ++f) {
        if (strcmp(f->format, format) == 0) {
            fmt = f;
            break;
        }
    }
    if (!fmt) {
        fprintf(stderr, "Error: Unsupported format: %s\n", format);
        return 1;
    }

    // Check if directory for single-file formats
    int is_dir = S_ISDIR(st.st_mode);
    if (fmt->format_id == ARCHIVE_FORMAT_EMPTY && is_dir) {
        fprintf(stderr, "Error: Format '%s' only supports single files, not directories\n", format);
        fprintf(stderr, "Use tar.gz, tar.xz, tar.bz2, tar.zst, zip, or 7z for directories\n");
        return 1;
    }

    // Generate output name
    char output_buf[PATH_MAX];
    char *dup_src = strdup(source);
    if (!dup_src) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        return 1;
    }
    char *base = basename(dup_src);
    snprintf(output_buf, sizeof(output_buf), "%s.%s", base, format);
    free(dup_src);
    char *output = output_buf;

    // Create archive
    struct archive *a = archive_write_new();
    if (!a) {
        fprintf(stderr, "Error: Failed to create archive\n");
        return 1;
    }

    archive_write_set_format(a, fmt->format_id);
    if (fmt->filter_id) {
        archive_write_add_filter(a, fmt->filter_id);
    }

    if (archive_write_open_filename(a, output) != ARCHIVE_OK) {
        fprintf(stderr, "Error: Failed to open output file: %s\n", archive_error_string(a));
        archive_write_free(a);
        return 1;
    }

    // Add files
    if (add_to_archive(a, source, is_dir ? NULL : source) != 0) {
        archive_write_free(a);
        return 1;
    }

    archive_write_close(a);
    archive_write_free(a);

    printf("Archive created successfully: %s\n", output);
    return 0;
}
