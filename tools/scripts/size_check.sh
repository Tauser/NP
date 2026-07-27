#!/usr/bin/env bash
# Gate de tamanho (docs/ARCHITECTURE.md §11 / ADR-013).
# Codigo curto nao e preferencia estetica: e o que mantem revisao viavel e
# bug localizavel.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
FW="$ROOT/firmware"
bad=0

[ -d "$FW" ] || { echo "size_check: sem firmware ainda, SKIP"; exit 0; }

check_lines() {
  local path="$1" limit="$2"
  [ -f "$path" ] || return 0
  local n; n="$(awk 'END{print NR}' "$path")"
  if [ "$n" -gt "$limit" ]; then
    echo "ERRO: $(realpath --relative-to="$ROOT" "$path") tem $n linhas (limite $limit)" >&2
    bad=1
  fi
}

check_lines "$FW/main/app_main.cpp" 300

while IFS= read -r f; do
  case "$f" in *"/screens/"*) check_lines "$f" 200 ;; *) check_lines "$f" 400 ;; esac
done < <(find "$FW" \( -name '*.cpp' -o -name '*.hpp' -o -name '*.c' -o -name '*.h' \) \
              -not -path '*/build/*' -not -path '*/managed_components/*' 2>/dev/null)

# Funcao longa: heuristica barata por bloco entre chaves na coluna 0.
while IFS= read -r f; do
  awk -v F="$f" '
    /^[a-zA-Z_].*\{[[:space:]]*$/ { start=NR; name=$0; depth=1; next }
    depth>0 { depth += gsub(/\{/,"{"); depth -= gsub(/\}/,"}")
              if (depth<=0) { if (NR-start > 40)
                  printf "AVISO: %s:%d funcao com %d linhas (limite 40)\n", F, start, NR-start > "/dev/stderr"
                depth=0 } }
  ' "$f"
done < <(find "$FW" -name '*.cpp' -not -path '*/build/*' \
              -not -path '*/managed_components/*' 2>/dev/null)

[ "$bad" -eq 0 ] || exit 1
echo "size_check: PASS"
