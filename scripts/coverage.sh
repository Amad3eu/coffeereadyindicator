#!/usr/bin/env bash
# Line coverage of the platform-independent layers (src/core, src/hardware,
# src/application) measured by the native test suite.
#
#   scripts/coverage.sh            # text summary; exit 1 below the thresholds
#   scripts/coverage.sh --html     # also writes docs/generated/coverage/index.html
#
# Requires PlatformIO and gcovr (`pip install platformio gcovr`).
set -euo pipefail

cd "$(dirname "$0")/.."

MIN_LINES="${MIN_LINES:-100}"
MIN_BRANCHES="${MIN_BRANCHES:-70}"
BUILD_DIR=".pio/build/native-coverage"
HTML_DIR="docs/generated/coverage"

for tool in pio gcovr; do
    if ! command -v "$tool" >/dev/null 2>&1; then
        echo "error: $tool not found (pip install platformio gcovr)" >&2
        exit 2
    fi
done

rm -rf "$BUILD_DIR"
pio test -e native-coverage

ARGS=(--root . --filter 'src/' --object-directory "$BUILD_DIR"
      --exclude-throw-branches --exclude-unreachable-branches
      --print-summary --txt
      --fail-under-line "$MIN_LINES" --fail-under-branch "$MIN_BRANCHES")

if [[ "${1:-}" == "--html" ]]; then
    mkdir -p "$HTML_DIR"
    ARGS+=(--html-details "$HTML_DIR/index.html")
fi

gcovr "${ARGS[@]}"
