#!/usr/bin/env bash
# Gate de higiene: o que nunca pode entrar no repositorio.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
cd "$ROOT"
bad=0

deny_tracked() {
  local pattern="$1" why="$2"
  local hits
  hits="$(git ls-files -- "$pattern" 2>/dev/null || true)"
  if [ -n "$hits" ]; then
    echo "ERRO: $why" >&2
    echo "$hits" | sed 's/^/  /' >&2
    bad=1
  fi
}

deny_tracked '*/build/*'              'build/ nao entra no repositorio'
deny_tracked 'firmware/sdkconfig'     'sdkconfig gerado nao entra (o .defaults sim)'
deny_tracked '*/managed_components/*' 'managed_components/ nao entra (o .lock sim)'
deny_tracked '*.pem'                  'chave no repositorio'
deny_tracked '*.key'                  'chave no repositorio'
deny_tracked '*secure_boot_signing*'  'chave de assinatura no repositorio'
deny_tracked '*.coredump'             'coredump nao entra'
deny_tracked '__pycache__/*'          'cache do Python nao entra'

# dependencies.lock DEVE estar versionado (ADR-022).
if [ -f firmware/dependencies.lock ] && ! git ls-files --error-unmatch \
     firmware/dependencies.lock >/dev/null 2>&1; then
  echo "ERRO: firmware/dependencies.lock existe mas nao esta versionado (ADR-022)" >&2
  bad=1
fi

[ "$bad" -eq 0 ] || exit 1
echo "hygiene: PASS"
