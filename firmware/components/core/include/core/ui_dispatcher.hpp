// UiDispatcher — converte FATOS em invalidação de UI
// (docs/UI-PATTERN.md §3, ADR-006: "o despachante é o dono único do coalescing").
//
// PURO: vive em `core` e NÃO conhece LVGL, telas nem hardware. Um alvo é
// `{máscara, callback, contexto}` — quem implementa a tela é a camada `ui`.
// Isso é o que permite testar toda a política de invalidação no host.
//
// PROPRIEDADE (ADR-008): tocado SÓ pela `app_loop`, que é quem publica no
// EventBus e quem chama `dispatch()`. Sem lock, de propósito: uma única task.
//
// DOIS COALESCINGS, que resolvem coisas diferentes:
//   1. `StateStore` — junta mutações vindas de tasks diferentes entre dois
//      ticks (20 mudanças de relógio => 1 evento).
//   2. AQUI — junta EVENTOS DIFERENTES num ciclo: se `ClockChanged` e
//      `NetworkChanged` acontecem no mesmo tick e a tela se interessa pelos
//      dois, `update()` é chamado UMA vez, não duas.
//
// Sem `std::function`: ponteiro de função + `void* ctx`, que não aloca
// (`static std::function` global congela o boot nesta placa).
#pragma once

#include <cstddef>
#include <cstdint>

#include "models/events.hpp"

namespace nova {
namespace core {

using InvalidateFn = void (*)(void* ctx);

class UiDispatcher {
public:
    static constexpr size_t kMaxTargets = 8;
    static constexpr size_t kInvalidTarget = static_cast<size_t>(-1);

    // Registra um alvo com a máscara MÍNIMA de eventos que o afetam
    // (UI-PATTERN §6 regra 2). Devolve o índice, ou kInvalidTarget se lotado —
    // o chamador DEVE tratar: um alvo perdido em silêncio vira tela que nunca
    // atualiza. Alvos nascem visíveis.
    size_t register_target(models::EventMask mask, InvalidateFn fn, void* ctx);

    // Assinatura para o EventBus. Só acumula o bit; não invalida nada ainda —
    // a invalidação acontece no `dispatch()`, uma vez por ciclo.
    static void on_event(models::Event e, void* self);

    // Chamado pela `app_loop` no fim do tick. Marca sujo quem tem interseção
    // com os eventos do ciclo e invalida os sujos VISÍVEIS, uma vez cada.
    // Alvo sujo INVISÍVEL permanece sujo e repinta ao ficar visível
    // (UI-PATTERN §3: "tela invisível suja marca dirty e repinta em on_enter").
    // Devolve quantos alvos foram invalidados.
    size_t dispatch();

    // Navegação. Ao ficar visível com pendência, invalida na hora — é o
    // `on_enter` do UI-PATTERN.
    void set_visible(size_t id, bool visible);

    bool is_dirty(size_t id) const;
    size_t target_count() const { return count_; }
    models::EventMask pending() const { return pending_; }
    uint32_t invalidations() const { return invalidations_; }

private:
    struct Target {
        models::EventMask mask_ = 0;
        InvalidateFn fn_ = nullptr;
        void* ctx_ = nullptr;
        bool visible_ = true;
        bool dirty_ = false;
    };

    void invalidate(Target& t);

    Target targets_[kMaxTargets];
    size_t count_ = 0;
    models::EventMask pending_ = 0;
    uint32_t invalidations_ = 0;
};

}  // namespace core
}  // namespace nova
