#include "services/network_worker_task.hpp"

#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace nova {
namespace services {

namespace {
constexpr const char* kTag = "net_worker";
constexpr uint32_t kTaskStackBytes = 8 * 1024;
constexpr uint32_t kTaskPriority = 3;
constexpr uint32_t kIdleDelayMs = 100;
}  // namespace

NetworkWorkerTask::NetworkWorkerTask(NetworkWorker& worker) : worker_(worker) {}

bool NetworkWorkerTask::start() {
    if (started_) {
        return true;
    }
    const size_t free_before = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
    const size_t largest_before = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL);
    body_storage_ = static_cast<uint8_t*>(heap_caps_malloc(
        utils::kHttpBodyMaxBytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
    if (body_storage_ == nullptr || !worker_.configure_body_storage(body_storage_, utils::kHttpBodyMaxBytes)) {
        ESP_LOGE(kTag, "sem 48 KiB internos p/ corpo HTTP (livre=%u KiB maior=%u KiB)",
                 static_cast<unsigned>(free_before / 1024),
                 static_cast<unsigned>(largest_before / 1024));
        return false;
    }
    if (xTaskCreate(task_entry, "net_worker", kTaskStackBytes, this, kTaskPriority, nullptr) != pdPASS) {
        ESP_LOGE(kTag, "nao criou net_worker (8 KiB)");
        return false;
    }
    started_ = true;
    ESP_LOGI(kTag, "SRAM interna: antes=%u KiB maior=%u KiB, apos corpo=%u KiB",
             static_cast<unsigned>(free_before / 1024), static_cast<unsigned>(largest_before / 1024),
             static_cast<unsigned>(heap_caps_get_free_size(MALLOC_CAP_INTERNAL) / 1024));
    return true;
}

void NetworkWorkerTask::task_entry(void* context) { static_cast<NetworkWorkerTask*>(context)->run(); }

void NetworkWorkerTask::run() {
    for (;;) {
        const utils::Status result = worker_.run_once(esp_timer_get_time() / 1000);
        if (result != utils::Status::kOk && result != utils::Status::kBusy) {
            ESP_LOGW(kTag, "request terminou: %s", utils::to_string(result));
        }
        vTaskDelay(pdMS_TO_TICKS(kIdleDelayMs));
    }
}

}  // namespace services
}  // namespace nova
