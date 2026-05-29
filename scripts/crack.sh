#!/usr/bin/env bash
# Run the Bombe cracker against one crib file or all cribs in a directory.
#
# Usage:
#   ./scripts/crack.sh                          # default: cribs/bombe_pc.toml
#   ./scripts/crack.sh cribs/crib_42.toml       # single file
#   ./scripts/crack.sh cribs/                   # all *.toml files in directory
#   ./scripts/crack.sh --ring-search            # search all 26² ring combinations
#   ./scripts/crack.sh cribs/ --ring UPP        # override ring for every file
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BIN="$REPO_ROOT/build/cli/bombe"

if [[ ! -f "$BIN" ]]; then
    echo "bombe not found — building first..."
    "$REPO_ROOT/scripts/build.sh"
fi

cd "$REPO_ROOT"

# Separate the positional target (file or dir) from bombe flags
TARGET=""
BOMBE_ARGS=()

for arg in "$@"; do
    if [[ -z "$TARGET" && "$arg" != --* ]]; then
        TARGET="$arg"
    else
        BOMBE_ARGS+=("$arg")
    fi
done

# Default target
[[ -z "$TARGET" ]] && TARGET="cribs/bombe_pc.toml"

# Directory: iterate all *.toml files
if [[ -d "$TARGET" ]]; then
    CRIBS=("$TARGET"/*.toml)
    if [[ ${#CRIBS[@]} -eq 0 || ! -f "${CRIBS[0]}" ]]; then
        echo "No .toml files found in $TARGET"
        exit 1
    fi
    for crib in "${CRIBS[@]}"; do
        echo "════════════════════════════════════════════════════════════"
        echo "Cracking: $crib"
        echo "════════════════════════════════════════════════════════════"
        "$BIN" "$crib" "${BOMBE_ARGS[@]}"
        echo ""
    done
else
    "$BIN" "$TARGET" "${BOMBE_ARGS[@]}"
fi