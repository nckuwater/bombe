#!/usr/bin/env bash
# Remove the build directory and generated crib files.
# Usage: ./scripts/clean.sh [--all]
#   --all   also removes cribs/ (keeps cribs/bombe_pc.toml by default)
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
CLEAN_CRIBS=false

for arg in "$@"; do
    [[ "$arg" == "--all" ]] && CLEAN_CRIBS=true
done

echo "Removing build/..."
rm -rf "$REPO_ROOT/build"

if $CLEAN_CRIBS; then
    echo "Removing cribs/..."
    rm -rf "$REPO_ROOT/cribs"
else
    echo "Removing cribs/crib_*.toml (keeping bombe_pc.toml)..."
    rm -f "$REPO_ROOT/cribs"/crib_*.toml
fi

echo "Done."
