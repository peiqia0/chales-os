#!/bin/sh

set -e

usage() {
  cat <<EOUT
Usage: $0 [-u | -a] SYMBOL

Options:
  -u    show likely symbol uses only
  -a    show all occurrences

Without an option, the script finds likely definitions first and falls back to all matches.
EOUT
}

if [ "$#" -lt 1 ] || [ "$#" -gt 2 ]; then
  usage
  exit 1
fi

MODE="definitions"
case "$1" in
  -u)
    MODE="uses"
    shift
    ;;
  -a)
    MODE="all"
    shift
    ;;
  -*)
    usage
    exit 1
    ;;
esac

if [ "$#" -ne 1 ]; then
  usage
  exit 1
fi

SYMBOL=$1

search_all() {
  grep -RInF --include="*.c" --include="*.h" --include="*.S" --include="*.s" --include="*.asm" --include="*.ld" "$SYMBOL" .
}

search_definitions() {
  grep -RInE --include="*.c" --include="*.h" --include="*.S" --include="*.s" --include="*.asm" --include="*.ld" \
    -e "^[[:space:]]*#define[[:space:]]+${SYMBOL}([[:space:]]|$)" \
    -e "^[[:space:]]*typedef[[:space:]]+.*[[:space:]]+${SYMBOL}[[:space:]]*;" \
    -e "^[[:space:]]*(static[[:space:]]+)?[[:alnum:]_\\*][[:alnum:]_\\*[:space:]]+[[:space:]]+${SYMBOL}[[:space:]]*\\(" \
    -e "^[[:space:]]*(static[[:space:]]+)?[[:alnum:]_\\*][[:alnum:]_\\*[:space:]]+[[:space:]]+${SYMBOL}[[:space:]]*=[[:space:]]*.*" \
    -e "^[[:space:]]*(extern[[:space:]]+)?[[:alnum:]_\\*][[:alnum:]_\\*[:space:]]+[[:space:]]+${SYMBOL}[[:space:]]*;" .
}

search_uses() {
  search_all | grep -vE "^[[:space:]]*#define[[:space:]]+${SYMBOL}([[:space:]]|$)" | \
    grep -vE "^[[:space:]]*typedef[[:space:]]+.*[[:space:]]+${SYMBOL}[[:space:]]*;" | \
    grep -vE "^[[:space:]]*(static[[:space:]]+)?[[:alnum:]_\\*][[:alnum:]_\\*[:space:]]+[[:space:]]+${SYMBOL}[[:space:]]*\\(" | \
    grep -vE "^[[:space:]]*(static[[:space:]]+)?[[:alnum:]_\\*][[:alnum:]_\\*[:space:]]+[[:space:]]+${SYMBOL}[[:space:]]*=[[:space:]]*.*" | \
    grep -vE "^[[:space:]]*(extern[[:space:]]+)?[[:alnum:]_\\*][[:alnum:]_\\*[:space:]]+[[:space:]]+${SYMBOL}[[:space:]]*;"
}

case "$MODE" in
  definitions)
    if search_definitions | grep -q .; then
      search_definitions
      exit 0
    fi
    printf 'No obvious definition found for %s. Showing all matches:\n\n' "$SYMBOL"
    search_all
    ;;
  uses)
    if search_uses | grep -q .; then
      search_uses
    else
      printf 'No uses found for %s; showing all matches instead:\n\n' "$SYMBOL"
      search_all
    fi
    ;;
  all)
    search_all
    ;;
  *)
    usage
    exit 1
    ;;
esac
