// Camada de LOG sobre dma_check (ADR-010, ADR-019). Alvo-only: usa esp_log.
// Separada de dma_safety.cpp para manter aquela unidade pura no host.
#include "board/dma_safety.hpp"

#include <cinttypes>

#include "esp_log.h"

namespace nova {
namespace board {

namespace {
constexpr const char* kTag = "board.dma";
}  // namespace

bool assert_dma_safe(uintptr_t base, size_t size, size_t cache_line, const char* what) {
    const DmaSafety r = dma_check(base, size, cache_line);
    if (r.safe()) {
        ESP_LOGI(kTag, "%s: DMA-safe base=0x%08" PRIxPTR " size=%u (linha=%u B)",
                 what, base, static_cast<unsigned>(size), static_cast<unsigned>(cache_line));
        return true;
    }
    if (!r.cache_line_known_) {
        ESP_LOGE(kTag, "%s: linha de cache DESCONHECIDA — impossível provar DMA-safe", what);
        return false;
    }
    ESP_LOGE(kTag,
             "%s: DMA-UNSAFE base=0x%08" PRIxPTR " size=%u base%%%u=%u size%%%u=%u",
             what, base, static_cast<unsigned>(size),
             static_cast<unsigned>(cache_line), static_cast<unsigned>(r.base_rem_),
             static_cast<unsigned>(cache_line), static_cast<unsigned>(r.size_rem_));
    return false;
}

}  // namespace board
}  // namespace nova
