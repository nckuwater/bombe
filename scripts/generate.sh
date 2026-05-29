#!/usr/bin/env bash
# Generate crib files with random Enigma settings.
# Usage: ./scripts/generate.sh [--count N] [--seed N] [--out-dir DIR]
#
# Examples:
#   ./scripts/generate.sh                      # one crib → cribs/bombe_pc.toml
#   ./scripts/generate.sh --count 10           # 10 cribs → cribs/crib_<seed>.toml
#   ./scripts/generate.sh --seed 42            # reproducible single crib
#   ./scripts/generate.sh --count 5 --out-dir cribs/batch
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BIN="$REPO_ROOT/build/cli/generate_crib"

if [[ ! -f "$BIN" ]]; then
    echo "generate_crib not found — building first..."
    "$REPO_ROOT/scripts/build.sh"
fi

cd "$REPO_ROOT"
"$BIN" "$@"
