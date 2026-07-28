#!/usr/bin/env bash
# Compila e roda os testes nativos da lógica pura (docs/TESTING.md). Chamado por
# host_check.sh. Sem ESP-IDF: só g++ com as unidades puras.
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
FW="$(cd "$HERE/../.." && pwd)"
CXX="${CXX:-g++}"
OUT="$(mktemp -d "${TMPDIR:-/tmp}/novapanel-nat.XXXXXX")"
trap 'rm -rf "$OUT"' EXIT
SHIM="$OUT/shim"
mkdir -p "$SHIM"
printf '%s\n' '#pragma once' '#define ESP_LOGD(...) do {} while (0)' \
  '#define ESP_LOGI(...) do {} while (0)' '#define ESP_LOGW(...) do {} while (0)' \
  '#define ESP_LOGE(...) do {} while (0)' > "$SHIM/esp_log.h"
printf '%s\n' '#pragma once' '#include <cstdint>' \
  'static inline int64_t esp_timer_get_time() { return 0; }' > "$SHIM/esp_timer.h"

INC=(
  -I"$SHIM"
  -I"$FW/components/board/include"
  -I"$FW/components/cache/include"
  -I"$FW/components/diag/include"
  -I"$FW/components/core/include"
  -I"$FW/components/models/include"
  -I"$FW/components/providers/include"
  -I"$FW/components/services/include"
  -I"$FW/components/ui/include"
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
  "$FW/components/core/src/service_manager.cpp" \
  "$FW/components/utils/src/status.cpp" \
  -o "$OUT/test_core"

"$CXX" -std="${CXXSTD:-c++17}" -Wall -Wextra -Werror "${INC[@]}" \
  "$HERE/test_service_manager.cpp" \
  "$FW/components/core/src/service_manager.cpp" \
  "$FW/components/utils/src/status.cpp" \
  -o "$OUT/test_service_manager"

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

"$CXX" -std="${CXXSTD:-c++17}" -Wall -Wextra -Werror "${INC[@]}" \
  "$HERE/test_time_provider.cpp" \
  "$FW/components/providers/src/mock_time_provider.cpp" \
  "$FW/components/utils/src/status.cpp" \
  -o "$OUT/test_time_provider"

"$CXX" -std="${CXXSTD:-c++17}" -Wall -Wextra -Werror "${INC[@]}" \
  "$HERE/test_open_meteo_weather_provider.cpp" \
  "$FW/components/providers/src/open_meteo_weather_provider.cpp" \
  "$FW/components/utils/src/status.cpp" \
  -o "$OUT/test_open_meteo_weather_provider"

"$CXX" -std="${CXXSTD:-c++17}" -Wall -Wextra -Werror "${INC[@]}" \
  "$HERE/test_weather_cache.cpp" \
  "$FW/components/cache/src/weather_cache_codec.cpp" \
  "$FW/components/utils/src/status.cpp" \
  -o "$OUT/test_weather_cache"

"$CXX" -std="${CXXSTD:-c++17}" -Wall -Wextra -Werror "${INC[@]}" \
  "$HERE/test_weather_service.cpp" \
  "$FW/components/core/src/state_store.cpp" \
  "$FW/components/core/src/request_orchestrator.cpp" \
  "$FW/components/cache/src/weather_cache_codec.cpp" \
  "$FW/components/services/src/network_worker.cpp" \
  "$FW/components/services/src/weather_service.cpp" \
  "$FW/components/utils/src/http_client.cpp" \
  "$FW/components/utils/src/status.cpp" \
  -o "$OUT/test_weather_service"

"$CXX" -std="${CXXSTD:-c++17}" -Wall -Wextra -Werror "${INC[@]}" \
  "$HERE/test_clock_service.cpp" \
  "$FW/components/core/src/state_store.cpp" \
  "$FW/components/services/src/clock_service.cpp" \
  -o "$OUT/test_clock_service"

"$CXX" -std="${CXXSTD:-c++17}" -Wall -Wextra -Werror "${INC[@]}" \
  "$HERE/test_setup_service.cpp" \
  "$FW/components/core/src/state_store.cpp" \
  "$FW/components/services/src/setup_service.cpp" \
  "$FW/components/services/src/wifi_credentials_store_mock.cpp" \
  "$FW/components/services/src/wifi_provisioning_mailbox.cpp" \
  "$FW/components/services/src/usb_wifi_provisioning_protocol.cpp" \
  -o "$OUT/test_setup_service"

"$CXX" -std="${CXXSTD:-c++17}" -Wall -Wextra -Werror "${INC[@]}" \
  "$HERE/test_screen_registry.cpp" \
  "$FW/components/ui/src/screen_registry.cpp" \
  -o "$OUT/test_screen_registry"

"$OUT/test_all"
"$OUT/test_core"
"$OUT/test_service_manager"
"$OUT/test_utils"
"$OUT/test_request_orchestrator"
"$OUT/test_http_client"
"$OUT/test_network_worker"
"$OUT/test_time_provider"
"$OUT/test_open_meteo_weather_provider"
"$OUT/test_weather_cache"
"$OUT/test_weather_service"
"$OUT/test_clock_service"
"$OUT/test_setup_service"
"$OUT/test_screen_registry"
