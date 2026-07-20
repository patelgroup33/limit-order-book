#!/usr/bin/env bash
#
# build.sh — configure and build the whole project (library, tests, simulator).
#
# Usage:
#   scripts/build.sh [BUILD_TYPE]      # BUILD_TYPE defaults to Release
#
# Examples:
#   scripts/build.sh                   # Release build
#   scripts/build.sh Debug             # Debug build
#
set -euo pipefail

# Repo root = the parent of this script's directory, so it works from anywhere.
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT}/build"
BUILD_TYPE="${1:-Release}"

echo "==> Configuring (${BUILD_TYPE}) in ${BUILD_DIR}"
cmake -S "${ROOT}" -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE="${BUILD_TYPE}"

echo "==> Building"
cmake --build "${BUILD_DIR}" --config "${BUILD_TYPE}" --parallel

echo "==> Done."
echo "    tests:     ${BUILD_DIR}/tests/lob_tests"
echo "    simulator: ${BUILD_DIR}/sim/lob_sim"
