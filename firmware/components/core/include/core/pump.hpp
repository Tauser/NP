// Um passo do ciclo da `app_loop`: drenar → publicar → despachar.
//
// Existe como função PURA para que o firmware e os testes rodem exatamente o
// MESMO código. Se o `app_loop` reimplementasse esta sequência, o teste passaria
// a validar uma cópia — e cópias divergem.
//
// ORDEM, que é o contrato:
//   1. `take_pending_events()` — recolhe, sob lock, os fatos que QUALQUER task
//      registrou desde o último tick, e zera a máscara;
//   2. publica cada fato no EventBus — **só aqui**, numa única task, o que é o
//      que torna verdadeiro o contrato single-threaded do EventBus;
//   3. `dispatch()` — invalida cada tela suja e visível UMA vez, mesmo que
//      vários eventos do ciclo a tenham sujado.
#pragma once

#include <cstddef>

#include "core/event_bus.hpp"
#include "core/state_store.hpp"
#include "core/ui_dispatcher.hpp"

namespace nova {
namespace core {

// Devolve quantos alvos de UI foram invalidados neste passo (0 quando nada
// mudou — o caso comum, e o que mantém o custo de render baixo).
size_t pump_once(StateStore& store, EventBus& bus, UiDispatcher& dispatcher);

}  // namespace core
}  // namespace nova
