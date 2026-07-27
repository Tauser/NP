// Diagnóstico de boot (alvo). Ver diag/boot_diag.hpp.
#include "diag/boot_diag.hpp"

#include <cinttypes>

#include "board/dma_safety.hpp"
#include "diag/boot_report.hpp"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_psram.h"
#include "sdkconfig.h"

namespace nova {
namespace diag {

namespace {
constexpr const char* kTag = "diag.boot";

// Linha de cache L2 do P4. Se o Kconfig não expuser a macro, fica 0 e o
// diagnóstico marca "desconhecida" em vez de fingir um valor (GLITCH §2.1).
#if defined(CONFIG_CACHE_L2_CACHE_LINE_SIZE)
constexpr size_t kCacheLine = CONFIG_CACHE_L2_CACHE_LINE_SIZE;
#else
constexpr size_t kCacheLine = 0;
#endif

#if defined(CONFIG_LV_DRAW_BUF_ALIGN)
constexpr size_t kDrawBufAlign = CONFIG_LV_DRAW_BUF_ALIGN;
#else
constexpr size_t kDrawBufAlign = 1;
#endif

void log_platform() {
    esp_chip_info_t info;
    esp_chip_info(&info);
    ESP_LOGI(kTag, "alvo=%s cores=%d silicio_rev=%d.%d",
             CONFIG_IDF_TARGET, info.cores, info.revision / 100, info.revision % 100);
    ESP_LOGI(kTag, "PSRAM total=%u KB", static_cast<unsigned>(esp_psram_get_size() / 1024));
    ESP_LOGI(kTag, "heap interno livre=%u KB (maior bloco=%u KB)",
             static_cast<unsigned>(heap_caps_get_free_size(MALLOC_CAP_INTERNAL) / 1024),
             static_cast<unsigned>(heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL) / 1024));
    ESP_LOGI(kTag, "CONFIG_LV_DRAW_BUF_ALIGN=%u  CACHE_L2_LINE=%u B%s",
             static_cast<unsigned>(kDrawBufAlign), static_cast<unsigned>(kCacheLine),
             kCacheLine == 0 ? " (DESCONHECIDA)" : "");
}

// Defeito nº 1 (ADR-024): decide, SEM adivinhação, se este hardware admite a
// única mitigação real (auto-suspend do erase). Ver GLITCH-PROTOCOLO / ADR-024.
void log_flash_suspend_capability() {
    uint32_t id = 0;
    if (esp_flash_read_id(nullptr, &id) != ESP_OK) {
        ESP_LOGW(kTag, "flash: nao foi possivel ler o chip id");
        return;
    }
    const bool can = flash_supports_suspend(id);
#if defined(CONFIG_SPI_FLASH_AUTO_SUSPEND)
    const bool enabled = true;
#else
    const bool enabled = false;
#endif
    ESP_LOGI(kTag, "flash chip_id=0x%06X  auto-suspend: suportado=%s  ligado=%s",
             static_cast<unsigned>(id), can ? "SIM" : "NAO", enabled ? "SIM" : "nao");
    if (!can) {
        ESP_LOGW(kTag, "flash NAO admite suspend -> defeito nº1 so tem mitigacao "
                       "por POLITICA (minimizar erase); ver ADR-024");
    }
}

bool log_one_buffer(int idx, uintptr_t base, size_t size) {
    const size_t line = kCacheLine == 0 ? 1 : kCacheLine;
    ESP_LOGI(kTag, "draw_buf[%d] base=0x%08" PRIxPTR " size=%u B  base%%64=%u  size%%64=%u",
             idx, base, static_cast<unsigned>(size),
             static_cast<unsigned>(base % line), static_cast<unsigned>(size % line));

    const bool safe = board::assert_dma_safe(base, size, kCacheLine, "draw_buf");
    const AlignVerdict v = classify_alignment(kDrawBufAlign, base, kCacheLine);
    ESP_LOGI(kTag, "draw_buf[%d] veredito 2.1: %s", idx, verdict_text(v));
    return safe;
}
}  // namespace

bool run_boot_diagnostic(board::IBoard& board) {
    log_platform();
    log_flash_suspend_capability();

    const board::DrawBufferReport rep = board.describe_draw_buffers();
    if (rep.count_ == 0) {
        ESP_LOGE(kTag, "nenhum draw buffer reportado — display nao iniciou?");
        return false;
    }

    bool all_safe = true;
    for (size_t i = 0; i < rep.count_; ++i) {
        if (!log_one_buffer(static_cast<int>(i), rep.buffers_[i].base_, rep.buffers_[i].size_)) {
            all_safe = false;
        }
    }
    return all_safe;
}

}  // namespace diag
}  // namespace nova
