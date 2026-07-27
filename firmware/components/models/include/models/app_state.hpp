// AppState — os fatos que a UI desenha (docs/ARCHITECTURE.md §6).
//
// PROPRIEDADE, declarada aqui porque o header é o lugar contratual (ADR-008):
//
//   ESCRITOR ÚNICO: `core::StateStore`, e somente pelos seus setters. Nenhum
//   outro componente muta estes campos — nem services, nem UI, nem a HAL.
//   LEITORES: recebem CÓPIA por valor via acessores granulares do StateStore.
//   Não existe getter que devolva esta struct inteira (ADR-011) e nenhum leitor
//   recebe referência ou ponteiro para ela.
//   SERIALIZAÇÃO: pelo ILock injetado no StateStore. Comentário afirmando
//   segurança sem mecanismo é proibido — já houve data race real assim.
//
// PURO. Cresce por domínio conforme as ondas avançam; hoje só tem o que a
// Onda A justifica (ADR-023: nada de campo para dado que não existe).
//
// CUSTO DE PILHA: campo novo aqui custa PILHA da task de render, não só RAM.
// A rotina de render lê ACESSORES GRANULARES e nunca copia esta struct inteira
// (ADR-011) — copiar já causou stack protection fault em campo
// (PATRIMONIO-TECNICO §5.1). Antes de crescer, refaça a conta do
// RESOURCE-BUDGET §2.1.
#pragma once

#include <cstdint>

namespace nova {
namespace models {

enum class NetworkState : uint8_t {
    kDown = 0,
    kConnecting,
    kUp,
};

struct ClockState {
    // Hora local já resolvida. `valid_` falso = sem hora plausível; a UI mostra
    // estado não-sincronizado e NUNCA inventa data (ADR-015).
    uint8_t hour_ = 0;
    uint8_t minute_ = 0;
    bool valid_ = false;
};

struct AppState {
    ClockState clock_;
    NetworkState network_ = NetworkState::kDown;
};

}  // namespace models
}  // namespace nova
