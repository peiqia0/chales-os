#!/bin/sh
# Converts a binary file into a C byte array, either as a standalone
# header or as just the array body (for combining several programs into
# one header — see userland/Makefile's programs.h rule).
#
# Usage:
#   ./bin2header.sh <input-binary> <array-name>          > <output.h>
#   ./bin2header.sh <input-binary> <array-name> --body   >> <combined.h>
#
# --body emits only the array + length declarations (no #ifndef guard,
# no #include lines), so several of these can be concatenated under one
# shared guard. Without --body it's a complete, standalone header.
#
# Deliberately avoids depending on `xxd` (not always installed) — uses
# od + awk, which are POSIX and present everywhere make/gcc are.

set -e

if [ $# -lt 2 ] || [ $# -gt 3 ]; then
    echo "usage: $0 <input-binary> <array-name> [--body]" >&2
    exit 1
fi

INFILE="$1"
NAME="$2"
MODE="${3:-}"
GUARD=$(echo "$NAME" | tr '[:lower:]' '[:upper:]')_H

if [ ! -f "$INFILE" ]; then
    echo "error: '$INFILE' not found" >&2
    exit 1
fi

if [ "$MODE" = "--body" ]; then
    echo "/* From $INFILE. */"
elif [ -n "$MODE" ]; then
    echo "error: unrecognized option '$MODE'" >&2
    exit 1
else
    echo "/* Auto-generated from $INFILE by bin2header.sh — do not edit by hand.  */"
    echo "/* Regenerate after every rebuild of $INFILE.                          */"
    echo "#ifndef ${GUARD}"
    echo "#define ${GUARD}"
    echo
    echo "#include <stddef.h>"
    echo "#include <stdint.h>"
    echo
fi

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

if [ "$MODE" != "--body" ]; then
    echo
    echo "#endif /* ${GUARD} */"
fi
