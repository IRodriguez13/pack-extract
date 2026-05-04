#!/usr/bin/env bash

VERSION="1.5.2"
AUTHOR="Iván Ezequiel Rodriguez <ivanrwcm25@gmail.com>"

show_version() {
  echo "pack (pack-extract) $VERSION"
  echo "Copyright (C) 2026 Iván Ezequiel Rodriguez"
  echo "License GPLv3+: GNU GPL version 3 or later <https://gnu.org/licenses/gpl.html>."
  echo "This is free software: you are free to change and redistribute it."
  echo "There is NO WARRANTY, to the extent permitted by law."
  echo ""
  echo "Source: https://github.com/IRodriguez13/pack-extract"
  echo ""
  echo "Escrito por Iván Ezequiel Rodriguez."
}

show_usage() {
  echo "Usage: pack <format> <source>"
  echo "Packs files or directories into the specified format"
  echo ""
  echo "Supported formats:"
  echo "  tar, tar.gz, tar.xz, tar.bz2, tar.zst, tar.lz4, tar.lz, tar.lzo, tar.br"
  echo "  zip, 7z"
  echo "  gz, bz2, xz, zstd, lz4, lzo, br  (single file only)"
  echo ""
  echo "Options:"
  echo "  -v, --version    Show version information and exit"
  echo "  -h, --help       Show this help message and exit"
}

if [[ $# -eq 0 ]]; then
  show_usage
  exit 1
fi

if [[ "$1" == "--version" || "$1" == "-v" ]]; then
  show_version
  exit 0
fi

if [[ "$1" == "--help" || "$1" == "-h" ]]; then
  show_usage
  exit 0
fi

if [[ $# -lt 2 ]]; then
  show_usage
  exit 1
fi

format="$1"
shift
source="$1"

if [[ ! -e "$source" ]]; then
  echo "Error: Source not found: $source" >&2
  exit 1
fi

basename=$(basename "$source")

single_file_guard() {
  if [[ -d "$source" ]]; then
    echo "Error: Format '$format' only supports single files, not directories" >&2
    echo "Use tar.gz, tar.xz, tar.bz2, tar.zst, zip, or 7z for directories" >&2
    exit 1
  fi
}

case "$format" in
  tar)
    echo "Creating tar archive..."
    tar -cf "$basename.tar" "$source"
    ;;
  tar.gz)
    echo "Creating gzip compressed tar archive..."
    tar -czf "$basename.tar.gz" "$source"
    ;;
  tar.xz)
    echo "Creating xz compressed tar archive..."
    tar -cJf "$basename.tar.xz" "$source"
    ;;
  tar.bz2)
    echo "Creating bzip2 compressed tar archive..."
    tar -cjf "$basename.tar.bz2" "$source"
    ;;
  tar.zst)
    echo "Creating zstd compressed tar archive..."
    tar --zstd -cf "$basename.tar.zst" "$source"
    ;;
  tar.lz4)
    echo "Creating lz4 compressed tar archive..."
    tar --use-compress-program=lz4 -cf "$basename.tar.lz4" "$source"
    ;;
  tar.lz)
    echo "Creating lzip compressed tar archive..."
    tar --lzip -cf "$basename.tar.lz" "$source"
    ;;
  tar.lzo)
    echo "Creating lzop compressed tar archive..."
    tar --use-compress-program=lzop -cf "$basename.tar.lzo" "$source"
    ;;
  tar.br)
    echo "Creating brotli compressed tar archive..."
    tar --use-compress-program=brotli -cf "$basename.tar.br" "$source"
    ;;
  zip)
    echo "Creating zip archive..."
    zip -r "$basename.zip" "$source"
    ;;
  7z)
    echo "Creating 7z archive..."
    7z a "$basename.7z" "$source"
    ;;
  gz)
    single_file_guard
    echo "Compressing with gzip..."
    gzip -k "$source"
    ;;
  bz2)
    single_file_guard
    echo "Compressing with bzip2..."
    bzip2 -k "$source"
    ;;
  xz)
    single_file_guard
    echo "Compressing with xz..."
    xz -k "$source"
    ;;
  zstd)
    single_file_guard
    echo "Compressing with zstd..."
    zstd "$source" -o "$basename.zst"
    ;;
  lz4)
    single_file_guard
    echo "Compressing with lz4..."
    lz4 "$source" "$basename.lz4"
    ;;
  lzo)
    single_file_guard
    echo "Compressing with lzop..."
    lzop -o "$basename.lzo" "$source"
    ;;
  br)
    single_file_guard
    echo "Compressing with brotli..."
    brotli -o "$basename.br" "$source"
    ;;
  *)
    echo "Error: Unsupported format: $format" >&2
    echo "Supported formats: tar, tar.gz, tar.xz, tar.bz2, tar.zst, tar.lz4, tar.lz, tar.lzo, tar.br, zip, 7z, gz, bz2, xz, zstd, lz4, lzo, br" >&2
    exit 1
    ;;
esac

if [[ $? -eq 0 ]]; then
  echo "Archive created successfully"
else
  echo "Error: Failed to create archive" >&2
  exit 1
fi
