#!/usr/bin/env bash

file="$1"

if [[ $# -eq 0 ]]; then
  echo "Usage: extract <archive>"
  exit 1
fi

if [[ ! -f "$file" ]]; then
  echo "Error: File not found: $file" >&2
  exit 1
fi

# Snapshot antes
before=$(mktemp)
after=$(mktemp)

ls -1A > "$before"

mime=$(file --mime-type -b "$file")

case "$mime" in
  application/x-tar)
    tar -xf "$file"
    ;;
  application/gzip)
    tar -xzf "$file"
    ;;
  application/x-xz)
    tar -xJf "$file"
    ;;
  application/x-bzip2)
    tar -xjf "$file"
    ;;
  application/zip)
    unzip -qq "$file"
    ;;
  application/x-7z-compressed)
    7z x -bd "$file"
    ;;
  *)
    echo "Unsupported format: $mime" >&2
    rm "$before" "$after"
    exit 1
    ;;
esac

# Snapshot después
ls -1A > "$after"

echo
echo "Archive extracted successfully."
echo "Extract result:"

comm -13 <(sort "$before") <(sort "$after")

rm "$before" "$after"
