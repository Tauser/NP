#!/usr/bin/env bash
# Gate de regras de render (docs/UI-LAYOUT-SYSTEM.md §6).
# Cada regra aqui JA regrediu em silencio num baseline anterior.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
UI="$ROOT/firmware/components/ui"
bad=0

[ -d "$UI/src" ] || { echo "ui_check: sem UI ainda, SKIP"; exit 0; }

# Telas e layouts: onde as regras valem. Componentes e estilos sao a excecao
# autorizada, porque sao a camada que encapsula o LVGL.
SCOPE=()
[ -d "$UI/src/screens" ] && SCOPE+=("$UI/src/screens")
[ -d "$UI/src/layouts" ] && SCOPE+=("$UI/src/layouts")
[ "${#SCOPE[@]}" -gt 0 ] || { echo "ui_check: sem telas ainda, SKIP"; exit 0; }

deny() {
  local pattern="$1" rule="$2"
  if grep -RnE "$pattern" "${SCOPE[@]}" 2>/dev/null; then
    echo "ERRO: $rule" >&2
    bad=1
  fi
}

deny 'lv_obj_set_style_transform_' 'R4: transform_* forca draw layer'
deny 'shadow_width'                'R5: sombra e recalculada a cada repintura'
deny 'LV_SIZE_CONTENT'             'R2: tamanho-por-conteudo em conteudo dinamico'
deny 'lv_obj_set_style_'           'R10: estilo direto em tela (use componentes)'
deny 'static[[:space:]]+std::function' 'proibicao de plataforma: congela o boot'

# R1: criar objeto dentro de update_* significa geometria mudando em runtime.
if awk '/^[a-zA-Z_].*update_[a-zA-Z_]*\(/{i=1} i && /lv_(obj|label|line|bar)_create/{
          printf "%s:%d: %s\n", FILENAME, NR, $0; f=1 } /^\}/{i=0} END{exit f?0:1}' \
       $(find "${SCOPE[@]}" -name '*.cpp' 2>/dev/null) 2>/dev/null; then
  echo "ERRO: R1: criacao de widget dentro de update_() -- geometria e do build()" >&2
  bad=1
fi

[ "$bad" -eq 0 ] || exit 1
echo "ui_check: PASS"
