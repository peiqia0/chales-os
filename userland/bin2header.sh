#!/bin/sh
# Converts a binary file into a C header exposing it as a byte array,
# so it can be #include'd directly instead of objcopy-embedded.
#
# Usage: ./bin2header.sh <input-binary> <array-name> > <output.h>
# Example: ./bin2header.sh hello.elf hello_elf > ../kernel/include/kernel/hello_elf.h
#
# Deliberately avoids depending on `xxd` (not always installed) — uses
# od + awk, which are POSIX and present everywhere make/gcc are.

set -e

if [ $# -ne 2 ]; then
    echo "usage: $0 <input-binary> <array-name>" >&2
    exit 1
fi

INFILE="$1"
NAME="$2"
GUARD=$(echo "$NAME" | tr '[:lower:]' '[:upper:]')_H

if [ ! -f "$INFILE" ]; then
    echo "error: '$INFILE' not found" >&2
    exit 1
fi

echo "/* Auto-generated from $INFILE by bin2header.sh — do not edit by hand.  */"
echo "/* Regenerate after every rebuild of $INFILE.                          */"
echo "#ifndef ${GUARD}"
echo "#define ${GUARD}"
echo
echo "#include <stddef.h>"
echo "#include <stdint.h>"
echo
echo "static const uint8_t ${NAME}[] = {"

od -An -v -tx1 "$INFILE" | awk '
{
    for (i = 1; i <= NF; i++) {
        printf "0x%s, ", $i
        count++
        if (count % 12 == 0) printf "\n"
    }
}
END {
    if (count % 12 != 0) printf "\n"
}' | sed 's/^/    /; s/[ \t]*$//'

echo "};"
echo
echo "static const size_t ${NAME}_len = sizeof(${NAME});"
echo
echo "#endif /* ${GUARD} */"
