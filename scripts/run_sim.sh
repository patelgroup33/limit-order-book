#!/usr/bin/env bash
#
# run_sim.sh — build the simulator, generate a fresh replay, and open the
#              browser visualizer. This is the one command to "see it run".
#
# Usage:
#   scripts/run_sim.sh [STEPS] [PORT]
#     STEPS  number of order-flow steps to simulate (default 1200)
#     PORT   local port for the visualizer      (default 8137)
#
# Examples:
#   scripts/run_sim.sh                 # 1200 steps on http://localhost:8137
#   scripts/run_sim.sh 3000 9000       # a longer run on port 9000
#
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT}/build"
STEPS="${1:-1200}"
PORT="${2:-8137}"
URL="http://localhost:${PORT}"

echo "==> Building simulator"
# Tests off here so the simulator build needs no GoogleTest fetch — this path is
# just "see it run". Use scripts/build.sh or test.sh for the full project.
cmake -S "${ROOT}" -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE=Release -DLOB_BUILD_TESTS=OFF >/dev/null
cmake --build "${BUILD_DIR}" --target lob_sim --config Release --parallel

echo "==> Generating replay data (${STEPS} steps) -> web/simulation.json"
"${BUILD_DIR}/sim/lob_sim" "${STEPS}" "${ROOT}/web/simulation.json"

echo "==> Serving visualizer at ${URL}  (press Ctrl+C to stop)"
# Best-effort: open the browser a moment after the server starts.
(
  sleep 1
  if command -v open >/dev/null 2>&1; then open "${URL}"
  elif command -v xdg-open >/dev/null 2>&1; then xdg-open "${URL}"
  fi
) &

cd "${ROOT}/web"
# python3 on macOS/most Linux; fall back to python if that's what's installed.
if command -v python3 >/dev/null 2>&1; then
  python3 -m http.server "${PORT}"
else
  python -m http.server "${PORT}"
fi
