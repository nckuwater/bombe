#!/usr/bin/env bash
# Build (if needed) and run the full test suite.
# Usage: ./scripts/test.sh [--filter <pattern>]
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="$REPO_ROOT/build"
FILTER=""

while [[ $# -gt 0 ]]; do
    case "$1" in
        --filter) FILTER="$2"; shift 2 ;;
        *)        echo "Unknown arg: $1"; exit 1 ;;
    esac
done

if [[ ! -f "$BUILD_DIR/tests/bombe_tests" ]]; then
    echo "Test binary not found — building first..."
    "$REPO_ROOT/scripts/build.sh"
fi

cd "$REPO_ROOT"

if [[ -n "$FILTER" ]]; then
    "$BUILD_DIR/tests/bombe_tests" --gtest_filter="$FILTER"
else
    "$BUILD_DIR/tests/bombe_tests"
fi
