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

before=$(mktemp)
after=$(mktemp)

ls -1A > "$before"

mime=$(file --mime-type -b "$file")

is_tar_inside() {
  case "$file" in
    *.tar.*|*.tgz|*.tbz2|*.txz|*.tzst|*.tlz4) return 0 ;;
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
