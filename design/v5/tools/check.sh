#!/usr/bin/env bash
# Gate da referencia de design v5.
#
# NAO compila contra o LVGL real (nao ha toolchain garantido aqui). Confere
# tres coisas que dao para verificar estaticamente e que sao justamente as
# que regridem em silencio:
#
#   1. aritmetica da grade 12x8 fecha em 1024x600
#   2. os tokens de grade batem com os do firmware (ui_grid.hpp)
#   3. nenhuma tela viola R5 (sombra) ou R10 (lv_obj_set_style_* fora dos
#      componentes)
#
# Compilacao de verdade acontece no firmware: `idf.py build`.

set -eu

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
V5="$(cd "$SCRIPT_DIR/.." && pwd)"
ROOT="$(cd "$V5/../../.." && pwd)"
FW_GRID="$ROOT/firmware/components/ui/include/ui_grid.hpp"

bad=0
fail() { echo "ERRO: $*" >&2; bad=1; }

echo "v5 check"
echo "  pasta: $V5"

# ---- 1. aritmetica da grade ----------------------------------------------
h=$(( 64 + 12 * 60 + 11 * 16 + 64 ))
v=$(( 24 + 8 * 55 + 7 * 16 + 24 ))
[ "$h" -eq 1024 ] || fail "grade horizontal fecha em $h, nao 1024"
[ "$v" -eq 600 ]  || fail "grade vertical fecha em $v, nao 600"
echo "  OK  grade 12x8 fecha em ${h}x${v}"

# ---- 2. coerencia com o firmware -----------------------------------------
if [ -f "$FW_GRID" ]; then
  for pair in "kMarginH:NP_MARGIN_H:64" "kColW:NP_COL_UNIT:60" \
              "kGutterH:NP_GUTTER_H:16" "kMarginV:NP_MARGIN_V:24" \
              "kRowH:NP_ROW_UNIT:55" "kGutterV:NP_GUTTER_V:16"; do
    fw_name="${pair%%:*}"
    rest="${pair#*:}"
    v5_name="${rest%%:*}"
    want="${rest##*:}"

    fw_val=$(grep -oE "${fw_name} = [0-9]+" "$FW_GRID" | grep -oE '[0-9]+$' | head -1 || true)
    v5_val=$(grep -oE "define ${v5_name} [0-9]+" "$V5/core/np_tokens.h" \
             | grep -oE '[0-9]+$' | head -1 || true)

    [ "${fw_val:-x}" = "$want" ] || fail "firmware ${fw_name}=${fw_val:-ausente}, esperado $want"
    [ "${v5_val:-x}" = "$want" ] || fail "v5 ${v5_name}=${v5_val:-ausente}, esperado $want"
  done
  echo "  OK  tokens de grade batem com ui_grid.hpp"
else
  echo "  --  ui_grid.hpp ausente, pulando comparacao com o firmware"
fi

# ---- 3. regras de custo de render ----------------------------------------
if grep -rn "shadow_width" "$V5/screens" 2>/dev/null | grep -v '^\s*\*' | grep -q .; then
  fail "R5: sombra em arquivo de tela"
else
  echo "  OK  R5: nenhuma sombra em screens/"
fi

# R10: so np_components.c pode chamar lv_obj_set_style_*.
if grep -rn "lv_obj_set_style_" "$V5/screens" 2>/dev/null | grep -q .; then
  grep -rn "lv_obj_set_style_" "$V5/screens" >&2
  fail "R10: lv_obj_set_style_* em arquivo de tela (use np_components)"
else
  echo "  OK  R10: screens/ nao chama lv_obj_set_style_*"
fi

# R4: transform forca draw layer.
if grep -rn "transform_scale\|transform_angle" "$V5/screens" 2>/dev/null | grep -q .; then
  fail "R4: transform_* em arquivo de tela"
else
  echo "  OK  R4: nenhum transform_* em screens/"
fi

# ---- 4. toda tela do catalogo tem arquivo --------------------------------
missing=0
for id in boot home market setup weather timer agenda alarms \
          notifications devices settings sheets; do
  [ -f "$V5/screens/np_${id}.c" ] || { fail "faltando screens/np_${id}.c"; missing=1; }
done
[ "$missing" -eq 0 ] && echo "  OK  12 telas presentes"

# ---- 5. sintaxe C (sem LVGL) ---------------------------------------------
# Compilar de verdade exige lvgl.h. O que da para checar sem ele e
# balanceamento de chaves por arquivo -- barato e pega truncamento.
for f in "$V5"/core/*.c "$V5"/screens/*.c; do
  opens=$(tr -cd '{' < "$f" | wc -c)
  closes=$(tr -cd '}' < "$f" | wc -c)
  [ "$opens" -eq "$closes" ] || fail "chaves desbalanceadas em $(basename "$f") ($opens vs $closes)"
done
echo "  OK  chaves balanceadas em core/ e screens/"

if [ "$bad" -ne 0 ]; then
  echo "v5 check: FAIL" >&2
  exit 1
fi
echo "v5 check: PASS"
