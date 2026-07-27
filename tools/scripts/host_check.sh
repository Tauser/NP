#!/usr/bin/env bash
# Compilação de host do que não depende do alvo.
#
# Portátil de propósito: bash puro + g++ com shims. Roda igual em Linux, WSL
# e CI. Se falhar SÓ no seu ambiente, o bug é do script -- corrija o script,
# não pule a validação (docs/TESTING.md §3).
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
FIRMWARE="$ROOT/firmware"
CHECK_APP=0
CHECK_TESTS=0

while [ "$#" -gt 0 ]; do
  case "$1" in
    --app)   CHECK_APP=1 ;;
    --tests) CHECK_TESTS=1 ;;
    *) echo "uso: host_check.sh [--app] [--tests]" >&2; exit 2 ;;
  esac
  shift
done

echo "host_check"
echo "  root: $ROOT"

if [ ! -d "$FIRMWARE/components" ]; then
  echo "  sem componentes ainda (Onda A nao iniciada): PASS"
  exit 0
fi

CXX="${CXX:-g++}"
command -v "$CXX" >/dev/null 2>&1 || { echo "ERRO: $CXX nao encontrado" >&2; exit 1; }

WORK="$(mktemp -d "${TMPDIR:-/tmp}/novapanel-host.XXXXXX")"
trap 'rm -rf "$WORK"' EXIT
SHIM="$WORK/shim"
mkdir -p "$SHIM/freertos"

# Shims mínimos: o suficiente para compilar lógica pura sem o ESP-IDF.
cat > "$SHIM/esp_log.h" <<'SHIMEOF'
#pragma once
#define ESP_LOGD(...) do {} while (0)
#define ESP_LOGI(...) do {} while (0)
#define ESP_LOGW(...) do {} while (0)
#define ESP_LOGE(...) do {} while (0)
SHIMEOF

cat > "$SHIM/esp_timer.h" <<'SHIMEOF'
#pragma once
#include <cstdint>
extern "C" int64_t esp_timer_get_time(void);
SHIMEOF

cat > "$SHIM/freertos/FreeRTOS.h" <<'SHIMEOF'
#pragma once
#define pdMS_TO_TICKS(ms) (ms)
SHIMEOF

cat > "$SHIM/freertos/task.h" <<'SHIMEOF'
#pragma once
extern "C" void vTaskDelay(int);
SHIMEOF

INCLUDES=("-I$SHIM")
for d in "$FIRMWARE"/components/*/include; do
  [ -d "$d" ] && INCLUDES+=("-I$d")
done

MANIFEST="$FIRMWARE/tests/host_sources.txt"
if [ -f "$MANIFEST" ]; then
  while IFS= read -r rel || [ -n "$rel" ]; do
    case "$rel" in ''|'#'*) continue ;; esac
    src="$FIRMWARE/$rel"
    [ -f "$src" ] || { echo "ERRO: fonte ausente: $rel" >&2; exit 1; }
    "$CXX" -std="${CXXSTD:-c++17}" -Wall -Wextra -Werror "${INCLUDES[@]}" \
      -c "$src" -o "$WORK/$(basename "${rel%.*}").o"
    echo "  OK  firmware/$rel"
  done < "$MANIFEST"
else
  echo "  sem manifesto de fontes de host ainda"
fi

if [ "$CHECK_APP" -eq 1 ] && [ -f "$FIRMWARE/main/app_main.cpp" ]; then
  "$CXX" -std="${CXXSTD:-c++17}" -Wall -Wextra -Werror "${INCLUDES[@]}" \
    -fsyntax-only "$FIRMWARE/main/app_main.cpp"
  echo "  OK  firmware/main/app_main.cpp"
fi

if [ "$CHECK_TESTS" -eq 1 ] && [ -x "$FIRMWARE/tests/native/run_host_tests.sh" ]; then
  "$FIRMWARE/tests/native/run_host_tests.sh"
fi

echo "host_check: PASS"
