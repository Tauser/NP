// EventBus — publicação SÍNCRONA de fatos de domínio
// (docs/ARCHITECTURE.md §5 regra 2, ADR-006).
//
// PROPRIEDADE (ADR-008): esta classe NÃO é thread-safe por si. O handler roda
// na task de QUEM PUBLICOU, imediatamente, sem fila e sem troca de contexto.
// Quem publica de outra task deve fazê-lo sob o mesmo lock que protege os
// assinantes, ou publicar via ActionQueue e deixar a `app_loop` publicar aqui.
//
// Contrato do handler, herdado de defeito real:
//   - NÃO toca objetos LVGL (só a task de UI toca — ADR-011);
//   - NÃO bloqueia (roda na task do publicador, inclusive na `net_worker`);
//   - NÃO publica de volta durante o despacho (reentrância é rejeitada, ver
//     `publish`), porque isso já produziu recursão infinita em baseline anterior.
//
// Sem `std::function`: `static std::function` global congela o boot nesta
// plataforma (PATRIMONIO-TECNICO §2, proibição fatal). Assinatura é ponteiro de
// função + `void* ctx`, que não aloca.
#pragma once

#include <cstddef>
#include <cstdint>

#include "models/events.hpp"

namespace nova {
namespace core {

using EventHandler = void (*)(models::Event, void* ctx);

class EventBus {
public:
    // Capacidade fixa: sem alocação dinâmica em runtime. Assinantes são
    // registrados no wiring (boot) e nunca removidos em operação normal.
    static constexpr size_t kMaxSubscribers = 8;

    // Retorna false se não houver espaço — e o CHAMADOR deve tratar. Registrar
    // em silêncio um assinante a menos produziria tela que nunca atualiza.
    bool subscribe(EventHandler handler, void* ctx);

    // Despacha para todos os assinantes, na ordem de registro. Retorna o número
    // de handlers chamados. Publicação REENTRANTE (de dentro de um handler) é
    // ignorada e contada em `rejected_reentrant()` — nunca recursiona.
    size_t publish(models::Event e);

    size_t subscriber_count() const { return count_; }
    uint32_t published() const { return published_; }
    uint32_t rejected_reentrant() const { return rejected_reentrant_; }

private:
    struct Slot {
        EventHandler handler_ = nullptr;
        void* ctx_ = nullptr;
    };

    Slot slots_[kMaxSubscribers];
    size_t count_ = 0;
    bool dispatching_ = false;
    uint32_t published_ = 0;
    uint32_t rejected_reentrant_ = 0;
};

}  // namespace core
}  // namespace nova
