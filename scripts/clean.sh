#!/usr/bin/env bash
#
# clean.sh — remove all build directories (safe; they are regenerable).
#
# Usage:
#   scripts/clean.sh
#
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

for d in build build-bench build-docs; do
  if [ -d "${ROOT}/${d}" ]; then
    rm -rf "${ROOT:?}/${d}"
    echo "==> Removed ${d}/"
  fi
done
echo "==> Clean."
