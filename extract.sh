#!/usr/bin/env bash

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
VERSION="$(tr -d '[:space:]' < "$SCRIPT_DIR/VERSION")"
AUTHOR="Iván Ezequiel Rodriguez <ivanrwcm25@gmail.com>"

show_version() {
  echo "extract (pack-extract) $VERSION"
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
  echo "Usage: extract <archive>"
  echo "Extracts compressed archives automatically by detecting the format"
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

file="$1"

if [[ ! -f "$file" ]]; then
  echo "Error: File not found: $file" >&2
  exit 1
fi

before=$(mktemp)
after=$(mktemp)

ls -1A > "$before"

mime=$(file --mime-type -b "$file")

is_tar_inside() {
  case "$file" in
    *.tar.*|*.tgz|*.tbz2|*.txz|*.tzst|*.tlz4|*.tlzo|*.tbr) return 0 ;;
    *) return 1 ;;
  esac
}

case "$mime" in
  application/x-tar)
    tar -xf "$file"
    ;;
  application/gzip)
    if is_tar_inside; then
      tar -xzf "$file"
    else
      gunzip -k "$file"
    fi
    ;;
  application/x-xz)
    if is_tar_inside; then
      tar -xJf "$file"
    else
      unxz -k "$file"
    fi
    ;;
  application/x-bzip2)
    if is_tar_inside; then
      tar -xjf "$file"
    else
      bunzip2 -k "$file"
    fi
    ;;
  application/zstd|application/x-zstd)
    if is_tar_inside; then
      tar --zstd -xf "$file"
    else
      zstd -d -k "$file"
    fi
    ;;
  application/x-lz4)
    if is_tar_inside; then
      tar --use-compress-program=lz4 -xf "$file"
    else
      lz4 -d -k "$file"
    fi
    ;;
  application/x-lzip)
    tar --lzip -xf "$file"
    ;;
  application/x-lzop)
    if is_tar_inside; then
      tar --use-compress-program=lzop -xf "$file"
    else
      lzop -d -k "$file"
    fi
    ;;
  application/x-brotli)
    if is_tar_inside; then
      tar --use-compress-program=brotli -xf "$file"
    else
      brotli -d -k "$file"
    fi
    ;;
  application/zip)
    unzip -qq "$file"
    ;;
  application/x-7z-compressed)
    7z x -bd "$file"
    ;;
  application/x-rar|application/x-rar-compressed|application/vnd.rar)
    unrar x -o+ "$file"
    ;;
  application/x-cpio)
    cpio -id < "$file"
    ;;
  *)
    echo "Unsupported format: $mime" >&2
    rm "$before" "$after"
    exit 1
    ;;
esac

ls -1A > "$after"

echo
echo "Archive extracted successfully."
echo "Extract result:"

comm -13 <(sort "$before") <(sort "$after")

rm "$before" "$after"
