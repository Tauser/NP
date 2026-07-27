#!/usr/bin/env bash
# Roda todos os gates. E o que a task "Gate: todos" do VSCode chama e o que
# o CI executa.
set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
fail=0

for gate in arch_check size_check ui_check hygiene; do
  echo "── $gate ──────────────────────────────────────────"
  bash "$SCRIPT_DIR/$gate.sh" || fail=1
done

echo "── host_check ─────────────────────────────────────"
bash "$SCRIPT_DIR/host_check.sh" --app --tests || fail=1

echo
[ "$fail" -eq 0 ] && { echo "TODOS OS GATES: PASS"; exit 0; }
echo "ALGUM GATE FALHOU" >&2
exit 1
