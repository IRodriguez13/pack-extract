#!/usr/bin/env bash

if [[ $# -lt 2 ]]; then
  echo "Usage: pack <format> <source>"
  echo "Packs files or directories into the specified format"
  echo ""
  echo "Supported formats: tar, tar.gz, tar.xz, tar.bz2, zip"
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
  zip)
    echo "Creating zip archive..."
    zip -r "$basename.zip" "$source"
    ;;
  *)
    echo "Error: Unsupported format: $format" >&2
    echo "Supported formats: tar, tar.gz, tar.xz, tar.bz2, zip" >&2
    exit 1
    ;;
esac

if [[ $? -eq 0 ]]; then
  echo "Archive created successfully"
else
  echo "Error: Failed to create archive" >&2
  exit 1
fi
