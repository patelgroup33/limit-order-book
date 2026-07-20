#!/usr/bin/env bash
#
# test.sh — build the project, then run the full unit-test suite.
#
# Usage:
#   scripts/test.sh
#
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

# Reuse build.sh so building and testing never drift apart.
"${ROOT}/scripts/build.sh"

echo "==> Running tests"
ctest --test-dir "${ROOT}/build" --output-on-failure
