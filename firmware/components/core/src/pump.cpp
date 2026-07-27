#include "core/pump.hpp"

#include "models/events.hpp"

namespace nova {
namespace core {

size_t pump_once(StateStore& store, EventBus& bus, UiDispatcher& dispatcher) {
    // 1. Recolhe os fatos acumulados por qualquer task, sob o lock do estado.
    const models::EventMask m = store.take_pending_events();

    // 2. Publica — ÚNICO ponto do sistema que chama publish(). A ordem é a da
    // enumeração, portanto determinística; sem isso, dois publicadores
    // concorrentes poderiam inverter eventos entre si.
    for (uint8_t i = 0; i < models::kEventCount; ++i) {
        const models::Event e = static_cast<models::Event>(i);
        if ((m & models::mask_of(e)) != 0) {
            bus.publish(e);
        }
    }

    // 3. Uma invalidação por tela suja e visível, por mais eventos que tenham
    // ocorrido no ciclo.
    return dispatcher.dispatch();
}

}  // namespace core
}  // namespace nova
