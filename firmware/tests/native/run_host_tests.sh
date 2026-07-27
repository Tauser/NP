#!/usr/bin/env bash
# Compila e roda os testes nativos da lógica pura (docs/TESTING.md). Chamado por
# host_check.sh. Sem ESP-IDF: só g++ com as unidades puras.
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
FW="$(cd "$HERE/../.." && pwd)"
CXX="${CXX:-g++}"
OUT="$(mktemp -d "${TMPDIR:-/tmp}/novapanel-nat.XXXXXX")"
trap 'rm -rf "$OUT"' EXIT

INC=(
  -I"$FW/components/board/include"
  -I"$FW/components/diag/include"
  -I"$FW/components/core/include"
  -I"$FW/components/models/include"
  -I"$FW/components/services/include"
  -I"$FW/components/utils/include"
)

"$CXX" -std="${CXXSTD:-c++17}" -Wall -Wextra -Werror "${INC[@]}" \
  "$HERE/test_all.cpp" \
  "$FW/components/board/src/dma_safety.cpp" \
  "$FW/components/diag/src/render_metrics.cpp" \
  "$FW/components/diag/src/boot_report.cpp" \
  -o "$OUT/test_all"

"$CXX" -std="${CXXSTD:-c++17}" -Wall -Wextra -Werror "${INC[@]}" \
  "$HERE/test_core.cpp" \
  "$FW/components/core/src/event_bus.cpp" \
  "$FW/components/core/src/state_store.cpp" \
  "$FW/components/core/src/action_queue.cpp" \
  "$FW/components/core/src/ui_dispatcher.cpp" \
  "$FW/components/core/src/pump.cpp" \
  "$FW/components/core/src/request_orchestrator.cpp" \
  "$FW/components/utils/src/status.cpp" \
  -o "$OUT/test_core"

"$CXX" -std="${CXXSTD:-c++17}" -Wall -Wextra -Werror "${INC[@]}" \
  "$HERE/test_utils.cpp" \
  "$FW/components/utils/src/status.cpp" \
  -o "$OUT/test_utils"

"$CXX" -std="${CXXSTD:-c++17}" -Wall -Wextra -Werror "${INC[@]}" \
  "$HERE/test_request_orchestrator.cpp" \
  "$FW/components/core/src/request_orchestrator.cpp" \
  "$FW/components/utils/src/status.cpp" \
  -o "$OUT/test_request_orchestrator"

"$CXX" -std="${CXXSTD:-c++17}" -Wall -Wextra -Werror "${INC[@]}" \
  "$HERE/test_http_client.cpp" \
  "$FW/components/utils/src/http_client.cpp" \
  "$FW/components/utils/src/status.cpp" \
  -o "$OUT/test_http_client"

"$CXX" -std="${CXXSTD:-c++17}" -Wall -Wextra -Werror "${INC[@]}" \
  "$HERE/test_network_worker.cpp" \
  "$FW/components/services/src/network_worker.cpp" \
  "$FW/components/core/src/request_orchestrator.cpp" \
  "$FW/components/utils/src/http_client.cpp" \
  "$FW/components/utils/src/status.cpp" \
  -o "$OUT/test_network_worker"

"$OUT/test_all"
"$OUT/test_core"
"$OUT/test_utils"
"$OUT/test_request_orchestrator"
"$OUT/test_http_client"
"$OUT/test_network_worker"
