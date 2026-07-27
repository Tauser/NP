// Gerador de conteúdo de render para a Onda 0 (docs/GLITCH-PROTOCOLO.md
// §3.1/§3.2). Constrói o "carimbo" (contador monotônico grande) e alterna
// conteúdo por um lv_timer para maximizar flush. Alvo-only.
//
// Todo o LVGL é tocado DENTRO do timer, que roda na lvgl_task (ADR-011). Este
// módulo NÃO cria widget fora da lvgl_task.
#pragma once

#include <cstdint>

namespace nova {
namespace diag {

// aggressive=true  -> modo "torture" (§3.2): churn a ~10 Hz.
// aggressive=false -> modo ocioso: atualização a ~1 Hz.
// period_ms fixa o ritmo do timer.
void start_render_workout(bool aggressive, uint32_t period_ms);

}  // namespace diag
}  // namespace nova
