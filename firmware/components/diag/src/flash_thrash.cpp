// Erase de flash durante render (alvo). Ver diag/flash_thrash.hpp.
#include "diag/flash_thrash.hpp"

#include "esp_err.h"
#include "esp_log.h"
#include "esp_partition.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace nova {
namespace diag {

namespace {
constexpr const char* kTag = "diag.flash";
constexpr size_t kSector = 4096;

const esp_partition_t* g_part = nullptr;
uint32_t g_period_ms = 0;

void thrash_task(void*) {
    const size_t sectors = g_part->size / kSector;
    size_t s = 0;
    for (;;) {
        const int64_t t0 = esp_timer_get_time();
        const esp_err_t err = esp_partition_erase_range(g_part, s * kSector, kSector);
        const int64_t dt = esp_timer_get_time() - t0;
        // ERASE bloqueia o barramento MSPI; se o artefato piscar AGORA, casa com
        // este marcador. Prioridade abaixo da lvgl_task — a contenção é de
        // barramento (hardware), não de CPU.
        ESP_LOGW(kTag, "ERASE setor %u/%u -> %s em %lld us  <== durante render",
                 static_cast<unsigned>(s), static_cast<unsigned>(sectors),
                 esp_err_to_name(err), static_cast<long long>(dt));
        s = (s + 1) % sectors;
        vTaskDelay(pdMS_TO_TICKS(g_period_ms));
    }
}
}  // namespace

void start_flash_thrash(uint32_t period_ms) {
    g_part = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, "storage");
    if (g_part == nullptr) {
        ESP_LOGE(kTag, "particao 'storage' nao encontrada; flash-thrash OFF");
        return;
    }
    g_period_ms = period_ms;
    xTaskCreate(thrash_task, "flash_thrash", 4096, nullptr, 3, nullptr);
    ESP_LOGW(kTag, "FLASH-THRASH ligado: erase de 4KB a cada %u ms (hipotese 2.5)",
             static_cast<unsigned>(period_ms));
}

}  // namespace diag
}  // namespace nova
