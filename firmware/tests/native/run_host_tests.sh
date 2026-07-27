#!/usr/bin/env bash
# Compila e roda os testes nativos da lógica pura (docs/TESTING.md). Chamado por
# host_check.sh. Sem ESP-IDF: só g++ com as unidades puras.
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
FW="$(cd "$HERE/../.." && pwd)"
CXX="${CXX:-g++}"
OUT="$(mktemp -d "${TMPDIR:-/tmp}/novapanel-nat.XXXXXX")"
trap 'rm -rf "$OUT"' EXIT

"$CXX" -std="${CXXSTD:-c++17}" -Wall -Wextra -Werror \
  -I"$FW/components/board/include" \
  -I"$FW/components/diag/include" \
  "$HERE/test_all.cpp" \
  "$FW/components/board/src/dma_safety.cpp" \
  "$FW/components/diag/src/render_metrics.cpp" \
  "$FW/components/diag/src/boot_report.cpp" \
  -o "$OUT/test_all"

"$OUT/test_all"
