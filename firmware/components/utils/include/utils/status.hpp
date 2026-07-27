// Status — vocabulário único de falha (docs/ARCHITECTURE.md §7).
//
// PURO: sem IDF, sem alocação, sem exceções. Um enum pequeno, para caber em
// registrador e viajar de graça dentro de `Result<T>`.
//
// POR QUE UM ENUM E NÃO `esp_err_t`: o núcleo não conhece o IDF, e um código de
// erro de plataforma não distingue as coisas que ESTE produto precisa decidir —
// principalmente "corpo grande demais", que aqui é FALHA e não truncamento
// (RESOURCE-BUDGET §2 regra 2, ADR-004). A tradução de `esp_err_t` para cá
// acontece na borda (board/providers), não no núcleo.
#pragma once

#include <cstdint>

namespace nova {
namespace utils {

enum class Status : uint8_t {
    kOk = 0,

    // ── Falhas de transporte ────────────────────────────────────────────────
    kTimeout,        // não respondeu na janela
    kNetworkDown,    // sem conectividade; nem se tentou
    kBusy,           // 1 HTTPS por vez (RESOURCE-BUDGET §2 regra 1); tente depois
    kHttpError,      // respondeu, mas com status não-2xx

    // ── Falhas de conteúdo ──────────────────────────────────────────────────
    // Corpo excedeu o teto de 48 KB. É FALHA e conta no breaker: truncar em
    // silêncio produziria JSON cortado, que o parser rejeitaria depois com uma
    // mensagem que não aponta a causa real.
    kTooLarge,
    kMalformed,      // não fez parse, ou faltou campo obrigatório
    kStale,          // fez parse, mas o dado é velho demais para ser usado

    // ── Falhas locais ───────────────────────────────────────────────────────
    kNoMemory,
    kNotFound,
    kInvalidArg,
    kInternal,       // defeito nosso; não deveria acontecer
};

// Texto curto e ESTÁVEL, para log e para a tela de sistema. Estável porque vira
// string em log de campo, que é o que a triagem lê (OPERATIONS §3).
const char* to_string(Status s);

// Uma falha é "transitória" quando repetir mais tarde é razoável — orienta o
// backoff do orquestrador. `kMalformed` NÃO é: se o payload mudou de forma,
// repetir só gasta banda e bateria do provedor.
bool is_transient(Status s);

}  // namespace utils
}  // namespace nova
