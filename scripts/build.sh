#!/usr/bin/env bash
# Build the project. Run from the repo root.
# Usage: ./scripts/build.sh [Release|Debug]
set -euo pipefail

BUILD_TYPE="${1:-Release}"
BUILD_DIR="build"

cmake -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
cmake --build "$BUILD_DIR" --parallel

echo ""
echo "Binaries:"
echo "  ./build/cli/bombe"
echo "  ./build/cli/generate_crib"
echo "  ./build/tests/bombe_tests"
