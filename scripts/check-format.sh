#!/usr/bin/env bash
# Checks (or, with --fix, applies) clang-format on all C++ sources.
#
#   scripts/check-format.sh        # exit 1 if any file is not formatted
#   scripts/check-format.sh --fix  # rewrite files in place
#
# Requires clang-format >= 14. `pip install clang-format` provides a binary
# if your distribution does not.
set -euo pipefail

cd "$(dirname "$0")/.."

CLANG_FORMAT="${CLANG_FORMAT:-clang-format}"
if ! command -v "$CLANG_FORMAT" >/dev/null 2>&1; then
    echo "error: $CLANG_FORMAT not found (try: pip install clang-format)" >&2
    exit 2
fi

mapfile -t FILES < <(find src test include -type f \( -name '*.cpp' -o -name '*.h' \) | sort)

if [[ "${1:-}" == "--fix" ]]; then
    "$CLANG_FORMAT" -i "${FILES[@]}"
    echo "formatted ${#FILES[@]} files"
    exit 0
fi

if "$CLANG_FORMAT" --dry-run --Werror "${FILES[@]}"; then
    echo "format OK (${#FILES[@]} files)"
else
    echo "error: formatting differs; run 'make format'" >&2
    exit 1
fi
