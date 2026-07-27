#!/bin/bash
set -e

ROOT="${1:-.}"

find "$ROOT" \
    -type f \
    \( -name "*.c" -o -name "*.h" \) \
    -print0 |
while IFS= read -r -d '' file; do
    sed -E -i 's/\bprintf\b/printk/g' "$file"
done

echo "Done."
