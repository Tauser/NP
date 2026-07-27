// ActionQueue — intenções enfileiradas para a `app_loop` executar
// (docs/ARCHITECTURE.md §5 regra 3, ADR-008).
//
// Intenção é PEDIDO ("atualize o clima agora"); evento é FATO ("o clima mudou").
// Fato vai pelo EventBus, pedido vem por aqui.
//
// PROPRIEDADE (ADR-008): é escrita por qualquer task (touch, service, worker) e
// drenada SÓ pela `app_loop`. Portanto o acesso é serializado por um ILock
// injetado — o mesmo mecanismo do StateStore.
//
// O baseline anterior tinha fila de profundidade 4 com **descarte silencioso**:
// intenção sumia e ninguém sabia. Aqui a profundidade é >= 16 por contrato
// (static_assert) e todo overflow é CONTADO e recuperável por `overflows()`,
// para virar métrica em vez de mistério.
#pragma once

#include <cstddef>
#include <cstdint>

#include "core/lock.hpp"

namespace nova {
namespace core {

enum class Action : uint8_t {
    kRefreshClock = 0,
    kRetryNetwork,
    kCount,
};

class ActionQueue {
public:
    static constexpr size_t kCapacity = 16;
    static_assert(kCapacity >= 16, "ARCHITECTURE §5 regra 3 exige profundidade >= 16");

    explicit ActionQueue(ILock& lock) : lock_(lock) {}

    // Retorna false quando a fila está cheia; o descarte é CONTADO em
    // `overflows()`. O chamador não precisa tratar, mas o contador precisa
    // aparecer na instrumentação — fila que enche em silêncio esconde defeito.
    bool push(Action a);

    // Drena UMA intenção. Retorna false se vazia. A `app_loop` chama em laço
    // até esvaziar, no seu tick.
    bool pop(Action& out);

    size_t size() const;
    bool empty() const { return size() == 0; }
    uint32_t overflows() const;
    uint32_t pushed() const;

private:
    ILock& lock_;
    Action buf_[kCapacity] = {};
    size_t head_ = 0;   // próximo a sair
    size_t count_ = 0;
    uint32_t overflows_ = 0;
    uint32_t pushed_ = 0;
};

}  // namespace core
}  // namespace nova
