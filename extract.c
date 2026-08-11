/**
 * extract - Unified extraction tool rewrite in C
 * Copyright (C) 2026 Iván Ezequiel Rodriguez
 * License: GPLv3+
 *
 * Usage: extract <archive>
 * Extracts compressed archives automatically by detecting the format
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/stat.h>
#include <archive.h>
#include <archive_entry.h>
#include "version.h"

/* libarchive read block size (bytes); matches common examples in archive.h docs */
#define EXTRACT_BLOCK_SIZE 10240

void print_help(void)
{
    printf(
        "Usage: extract <archive>\n"
        "Extracts compressed archives automatically by detecting the format if supported.\n"
        "If a path already exists, asks whether to overwrite (Ctrl+C cancels).\n"
        "\n"
        "Options:\n"
        "  -v, --version    Show version information and exit\n"
        "  -h, --help       Show this help message and exit\n"
    );
}

void print_version(void)
{
    printf(
        "extract (pack-extract) %s\n"
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

/* Reject absolute paths and ".." components (zip-slip). */
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

static int cmp_strptr(const void *a, const void *b)
{
    return strcmp(*(const char *const *)a, *(const char *const *)b);
}

/*
 * Ask whether to overwrite an existing path.
 * Returns 1 = overwrite, 0 = skip, -1 = cancel / non-interactive conflict.
 * Ctrl+C delivers SIGINT (default: abort process). EOF / empty → skip.
 */
static int ask_overwrite(const char *pathname)
{
    char line[64];

    if (!isatty(STDIN_FILENO) || !isatty(STDOUT_FILENO))
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

/* Existing directory + directory entry: merge without prompting. */
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

int extract_archive(const char *filename)
{
    struct archive *a = NULL;
    struct archive *ext = NULL;
    char **extracted_files = NULL;
    size_t extracted_count = 0;
    struct archive_entry *entry;
    int r;
    int status = 1;

    a = archive_read_new();
    if (!a)
    {
        fprintf(stderr, "Error: Failed to create archive reader\n");
        return 1;
    }

    archive_read_support_format_all(a);
    archive_read_support_filter_all(a);

    if (archive_read_open_filename(a, filename, EXTRACT_BLOCK_SIZE) != ARCHIVE_OK)
    {
        fprintf(stderr, "Error: Failed to open archive: %s\n", archive_error_string(a));
        goto fail;
    }

    ext = archive_write_disk_new();
    if (!ext)
    {
        fprintf(stderr, "Error: Failed to create disk writer\n");
        goto fail;
    }

    archive_write_disk_set_options(ext,
        ARCHIVE_EXTRACT_TIME | ARCHIVE_EXTRACT_PERM |
        ARCHIVE_EXTRACT_ACL | ARCHIVE_EXTRACT_FFLAGS);
    archive_write_disk_set_standard_lookup(ext);

    while ((r = archive_read_next_header(a, &entry)) == ARCHIVE_OK)
    {
        const char *pathname = archive_entry_pathname(entry);
        char **new_list;
        const void *buff;
        size_t size;
        la_int64_t offset;

        if (!entry_path_is_safe(pathname))
        {
            fprintf(stderr, "Error: Refusing unsafe path in archive: %s\n",
                    pathname ? pathname : "(null)");
            goto fail;
        }

        if (path_exists(pathname) && !is_existing_dir_merge(pathname, entry))
        {
            int overwrite = ask_overwrite(pathname);

            if (overwrite < 0)
                goto fail;
            if (overwrite == 0)
            {
                printf("Skipping: %s\n", pathname);
                if (archive_read_data_skip(a) != ARCHIVE_OK)
                {
                    fprintf(stderr, "Error: Failed to skip entry: %s\n",
                            archive_error_string(a));
                    goto fail;
                }
                continue;
            }
        }

        printf("Extracting: %s\n", pathname);

        new_list = realloc(extracted_files, (extracted_count + 1) * sizeof(char *));
        if (!new_list)
        {
            fprintf(stderr, "Error: Memory allocation failed\n");
            goto fail;
        }
        extracted_files = new_list;
        extracted_files[extracted_count] = strdup(pathname);
        if (!extracted_files[extracted_count])
        {
            fprintf(stderr, "Error: Memory allocation failed\n");
            goto fail;
        }
        ++extracted_count;

        if (archive_write_header(ext, entry) != ARCHIVE_OK)
        {
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
    }

    if (r != ARCHIVE_EOF)
    {
        fprintf(stderr, "Error: Failed to read archive: %s\n", archive_error_string(a));
        goto fail;
    }

    if (extracted_count > 0)
    {
        qsort(extracted_files, extracted_count, sizeof(char *), cmp_strptr);

        printf("\nArchive extracted successfully.\nExtract result:\n");
        for (size_t i = 0; i < extracted_count; ++i)
        {
            printf("%s\n", extracted_files[i]);
            free(extracted_files[i]);
            extracted_files[i] = NULL;
        }
        free(extracted_files);
        extracted_files = NULL;
        extracted_count = 0;
    }
    else
    {
        printf("Archive extracted successfully\n");
    }

    status = 0;

fail:
    if (extracted_files)
    {
        for (size_t i = 0; i < extracted_count; ++i)
            free(extracted_files[i]);
        free(extracted_files);
        extracted_files = NULL;
    }
    if (ext)
    {
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
    if (argc == 2 && (strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-v") == 0))
    {
        print_version();
        return 0;
    }
    else if (argc == 2 && (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0))
    {
        print_help();
        return 0;
    }
    else if (argc != 2)
    {
        print_help();
        return 1;
    }

    const char *filename = argv[1];

    struct stat st;
    if (stat(filename, &st) != 0 || !S_ISREG(st.st_mode))
    {
        fprintf(stderr, "Error: File not found: %s\n", filename);
        return 1;
    }

    return extract_archive(filename);
}
