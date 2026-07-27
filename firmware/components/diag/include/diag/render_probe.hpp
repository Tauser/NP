// Sonda de render permanente (docs/GLITCH-PROTOCOLO.md §3.3): engancha nos
// eventos do LVGL para popular RenderMetrics e faz um dump periódico em INFO
// (flushes, duração p50/p95/máx, espera de flush, marca d'água da lvgl_task,
// heap interno e PSRAM). Alvo-only.
//
// Tudo roda na lvgl_task (ADR-011): os eventos de render e o timer de dump são
// disparados por ela. O único sinal cross-task é `first_frame_rendered()`.
#pragma once

#include "diag/render_metrics.hpp"

namespace nova {
namespace diag {

// Registra os callbacks de evento no display default e cria o timer de dump.
// `metrics` precisa ter vida longa (estático no app_main).
void attach_render_probe(RenderMetrics& metrics, uint32_t dump_period_ms);

// True depois que o PRIMEIRO frame foi efetivamente renderizado
// (LV_EVENT_RENDER_READY). É o gatilho para ligar o backlight (ARCHITECTURE §8).
// Atômico: escrito na lvgl_task, lido na task de boot.
bool first_frame_rendered();

}  // namespace diag
}  // namespace nova
