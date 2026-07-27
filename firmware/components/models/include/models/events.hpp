// Eventos de domínio (docs/ARCHITECTURE.md §5/§6).
//
// PURO: sem IDF, sem LVGL, sem FreeRTOS — `models` não depende de nada
// (imposto por arch_check.sh). Isso mantém todo o núcleo testável no host.
#pragma once

#include <cstdint>

namespace nova {
namespace models {

// Um evento é um FATO que já aconteceu, no passado — nunca um pedido.
// Pedido é intenção e trafega pela ActionQueue.
//
// Usado como índice de máscara de invalidação (ADR-006), por isso os valores
// são estáveis e kCount fecha a enumeração. Acrescentar evento no MEIO
// renumera a máscara: acrescente sempre ANTES de kCount.
enum class Event : uint8_t {
    kClockChanged = 0,   // minuto virou
    kNetworkChanged,     // conectividade mudou de estado
    kResourceWarning,    // limiar de RAM interna cruzado (RESOURCE-BUDGET §3)
    kCount,
};

constexpr uint8_t kEventCount = static_cast<uint8_t>(Event::kCount);

// Máscara de invalidação: bit N = Event N. Cabe em 32 bits com folga larga;
// se um dia kEventCount passar de 32, o static_assert avisa em vez de truncar
// em silêncio.
using EventMask = uint32_t;
static_assert(kEventCount <= 32, "EventMask precisa crescer alem de 32 bits");

constexpr EventMask mask_of(Event e) {
    return static_cast<EventMask>(1u) << static_cast<uint8_t>(e);
}

}  // namespace models
}  // namespace nova
