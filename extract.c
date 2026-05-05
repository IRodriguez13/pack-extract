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
#include <sys/stat.h>
#include <archive.h>
#include <archive_entry.h>

#define VERSION "1.5.2"

void print_help(void) {
    printf(
        "Usage: extract <archive>\n"
        "Extracts compressed archives automatically by detecting the format\n"
        "\n"
        "Options:\n"
        "  -v, --version    Show version information and exit\n"
        "  -h, --help       Show this help message and exit\n"
    );
}

void print_version(void) {
    printf(
        "extract (pack-extract) %s\n"
        "Copyright (C) 2026 Iván Ezequiel Rodriguez\n"
        "License GPLv3+: GNU GPL version 3 or later <https://gnu.org/licenses/gpl.html>.\n"
        "This is free software: you are free to change and redistribute it.\n"
        "There is NO WARRANTY, to the extent permitted by law.\n"
        "\n"
        "Source: https://github.com/IRodriguez13/pack-extract\n"
        "\n"
        "Escrito por Iván Ezequiel Rodriguez.\n",
        VERSION
    );
}

int extract_archive(const char *filename)
{
    struct archive *a = archive_read_new();
    if (!a)
    {
        fprintf(stderr, "Error: Failed to create archive reader\n");
        return 1;
    }

    archive_read_support_format_all(a);
    archive_read_support_filter_all(a);

    if (archive_read_open_filename(a, filename, 10240) != ARCHIVE_OK)
    {
        fprintf(stderr, "Error: Failed to open archive: %s\n", archive_error_string(a));
        archive_read_free(a);
        return 1;
    }

    struct archive *ext = archive_write_disk_new();
    if (!ext)
    {
        fprintf(stderr, "Error: Failed to create disk writer\n");
        archive_read_free(a);
        return 1;
    }

    archive_write_disk_set_options(ext, ARCHIVE_EXTRACT_TIME | ARCHIVE_EXTRACT_PERM | ARCHIVE_EXTRACT_ACL | ARCHIVE_EXTRACT_FFLAGS);
    archive_write_disk_set_standard_lookup(ext);

    // List of extracted files
    char **extracted_files = NULL;
    size_t extracted_count = 0;

    struct archive_entry *entry;
    int r;
    while ((r = archive_read_next_header(a, &entry)) == ARCHIVE_OK)
    {
        const char *pathname = archive_entry_pathname(entry);
        printf("Extracting: %s\n", pathname);

        // Add to list
        char **new_list = realloc(extracted_files, (extracted_count + 1) * sizeof(char *));
        if (!new_list)
        {
            fprintf(stderr, "Error: Memory allocation failed\n");
            // Free existing
            for (size_t i = 0; i < extracted_count; ++i)
            {
                free(extracted_files[i]);
            }
            free(extracted_files);
            archive_write_free(ext);
            archive_read_free(a);
            return 1;
        }
        extracted_files = new_list;
        extracted_files[extracted_count] = strdup(pathname);
        if (!extracted_files[extracted_count])
        {
            fprintf(stderr, "Error: Memory allocation failed\n");
            // Free
            for (size_t i = 0; i < extracted_count; ++i)
            {
                free(extracted_files[i]);
            }
            free(extracted_files);
            archive_write_free(ext);
            archive_read_free(a);
            return 1;
        }
        ++extracted_count;

        if (archive_write_header(ext, entry) != ARCHIVE_OK)
        {
            fprintf(stderr, "Error: Failed to write header: %s\n", archive_error_string(ext));
            // Free
            for (size_t i = 0; i < extracted_count; ++i)
            {
                free(extracted_files[i]);
            }
            free(extracted_files);
            archive_write_free(ext);
            archive_read_free(a);
            return 1;
        }

        const void *buff;
        size_t size;
        la_int64_t offset;
        while ((r = archive_read_data_block(a, &buff, &size, &offset)) == ARCHIVE_OK)
        {
            if (archive_write_data_block(ext, buff, size, offset) != ARCHIVE_OK)
            {
                fprintf(stderr, "Error: Failed to write data: %s\n", archive_error_string(ext));
                // Free
                for (size_t i = 0; i < extracted_count; ++i)
                {
                    free(extracted_files[i]);
                }
                free(extracted_files);
                archive_write_free(ext);
                archive_read_free(a);
                return 1;
            }
        }
        if (r != ARCHIVE_EOF)
        {
            fprintf(stderr, "Error: Failed to read data: %s\n", archive_error_string(a));
            // Free
            for (size_t i = 0; i < extracted_count; ++i)
            {
                free(extracted_files[i]);
            }
            free(extracted_files);
            archive_write_free(ext);
            archive_read_free(a);
            return 1;
        }
    }

    if (r != ARCHIVE_EOF)
    {
        fprintf(stderr, "Error: Failed to read archive: %s\n", archive_error_string(a));
        // Free
        for (size_t i = 0; i < extracted_count; ++i)
        {
            free(extracted_files[i]);
        }
        free(extracted_files);
        archive_write_free(ext);
        archive_read_free(a);
        return 1;
    }

    archive_write_free(ext);
    archive_read_free(a);

    // Sort and print extracted files
    if (extracted_count > 0)
    {
        // Sort
        qsort(extracted_files, extracted_count, sizeof(char *), (int (*)(const void *, const void *))strcmp);

        printf("\nArchive extracted successfully.\nExtract result:\n");
        for (size_t i = 0; i < extracted_count; ++i)
        {
            printf("%s\n", extracted_files[i]);
            free(extracted_files[i]);
        }
        free(extracted_files);
    }
    else
    {
        printf("Archive extracted successfully\n");
    }

    return 0;
}

int main(int argc, char *argv[])
{
    if (argc != 2)
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
        else
        {
            print_help();
            return 1;
        }
    }

    const char *filename = argv[1];

    // Check if file exists
    struct stat st;
    if (stat(filename, &st) != 0 || !S_ISREG(st.st_mode))
    {
        fprintf(stderr, "Error: File not found: %s\n", filename);
        return 1;
    }

    return extract_archive(filename);
}