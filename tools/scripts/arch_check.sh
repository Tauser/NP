#!/usr/bin/env bash
# Gate de camadas: a direcao de dependencia do docs/ARCHITECTURE.md §3 e
# imposta pelo build, nao por disciplina de review.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
FW="$ROOT/firmware"
bad=0

[ -d "$FW/components" ] || { echo "arch_check: sem componentes ainda, SKIP"; exit 0; }

requires_of() {
  awk 'BEGIN{i=0} /^[[:space:]]*REQUIRES[[:space:]]*$/{i=1;next}
       i && /^[[:space:]]*\)[[:space:]]*$/{i=0}
       i { sub(/#.*/,""); gsub(/^[[:space:]]+|[[:space:]]+$/,"");
           if ($0!="") print }' "$1"
}

assert_allowed() {
  local comp="$1"; shift
  local allowed=" $* "
  local f="$FW/components/$comp/CMakeLists.txt"
  [ -f "$f" ] || return 0
  while IFS= read -r dep; do
    case "$allowed" in
      *" $dep "*) ;;
      *) echo "ERRO: componente '$comp' depende de '$dep' (proibido)" >&2; bad=1 ;;
    esac
  done < <(requires_of "$f")
}

assert_no_include() {
  local comp="$1" pattern="$2"
  [ -d "$FW/components/$comp" ] || return 0
  if grep -RnE "$pattern" "$FW/components/$comp" 2>/dev/null; then
    echo "ERRO: componente '$comp' inclui camada proibida: $pattern" >&2
    bad=1
  fi
}

# Tabela normativa do ARCHITECTURE.md §3.
assert_allowed ui        core models lvgl
# services pode usar as bordas de rede do ESP-IDF, mas continua proibido de
# depender de UI ou de adaptador concreto de provider. A lista explícita evita
# que "serviço" vire passe livre para qualquer componente.
assert_allowed services  core models cache providers utils board esp_http_client esp-tls esp_timer mbedtls
assert_allowed providers models utils
assert_allowed core      models utils
assert_allowed models
assert_allowed cache     utils

assert_no_include ui        '"(i_board|mock_board|waveshare_board|.*_provider)\.hpp"'
assert_no_include providers '"(event_bus|state_store|ui_dispatcher|screen_|i_board)\.hpp"'
assert_no_include board     '"(event_bus|state_store|ui_dispatcher|screen_)\.hpp"'
assert_no_include models    '"(esp_|driver/|lvgl)'

[ "$bad" -eq 0 ] || exit 1
echo "arch_check: PASS"
