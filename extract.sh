#!/usr/bin/env bash

file="$1"

if [[ $# -eq 0 ]]; then
  echo "Usage: extract <archive>"
  echo "Extracts archives automatically detecting the format"
  exit 1
fi

if [[ ! -f "$file" ]]; then
  echo "Error: File not found: $file" >&2
  exit 1
fi

mime=$(file --mime-type -b "$file")

case "$mime" in
  application/x-tar)
    echo "Extracting tar archive..."
    tar -xf "$file"
    ;;
  application/gzip)
    echo "Extracting gzip compressed tar archive..."
    tar -xzf "$file"
    ;;
  application/x-xz)
    echo "Extracting xz compressed tar archive..."
    tar -xJf "$file"
    ;;
  application/x-bzip2)
    echo "Extracting bzip2 compressed tar archive..."
    tar -xjf "$file"
    ;;
  application/zip)
    echo "Extracting zip archive..."
    unzip "$file"
    ;;
  application/x-7z-compressed)
    echo "Extracting 7z archive..."
    7z x "$file"
    ;;
  *)
    echo "Error: Unsupported format: $mime" >&2
    exit 1
    ;;
esac

if [[ $? -eq 0 ]]; then
  echo "Archive extracted successfully"
else
  echo "Error: Failed to extract archive" >&2
  exit 1
fi
