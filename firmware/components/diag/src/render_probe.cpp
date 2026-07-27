// Sonda de render (alvo). Ver diag/render_probe.hpp.
#include "diag/render_probe.hpp"

#include <atomic>

#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_psram.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"

namespace nova {
namespace diag {

namespace {
constexpr const char* kTag = "diag.render";

// Estado da sonda. Tocado só na lvgl_task (eventos + timer de dump), exceto
// first_frame_ que é lido pela task de boot — por isso, atômico (ADR-008).
struct ProbeCtx {
    RenderMetrics* metrics_ = nullptr;
    int64_t render_t0_us_ = 0;
    int64_t wait_t0_us_ = 0;
    std::atomic<bool> first_frame_{false};
};
ProbeCtx g_ctx;

void on_render_event(lv_event_t* e) {
    RenderMetrics* m = g_ctx.metrics_;
    if (m == nullptr) {
        return;
    }
    switch (lv_event_get_code(e)) {
        case LV_EVENT_RENDER_START:
            g_ctx.render_t0_us_ = esp_timer_get_time();
            break;
        case LV_EVENT_RENDER_READY:
            m->on_update(esp_timer_get_time() - g_ctx.render_t0_us_);
            g_ctx.first_frame_.store(true, std::memory_order_release);
            break;
        case LV_EVENT_FLUSH_START:
            m->on_flush();
            break;
        case LV_EVENT_FLUSH_WAIT_START:
            g_ctx.wait_t0_us_ = esp_timer_get_time();
            break;
        case LV_EVENT_FLUSH_WAIT_FINISH:
            m->on_flush_wait(esp_timer_get_time() - g_ctx.wait_t0_us_);
            break;
        default:
            break;
    }
}

unsigned lvgl_task_stack_free_bytes() {
    TaskHandle_t h = xTaskGetHandle("taskLVGL");
    if (h == nullptr) {
        return 0;  // handle indisponível: reporta 0 e o log diz "n/d"
    }
    return static_cast<unsigned>(uxTaskGetStackHighWaterMark(h) * sizeof(StackType_t));
}

void dump_cb(lv_timer_t* t) {
    RenderMetrics* m = static_cast<RenderMetrics*>(lv_timer_get_user_data(t));
    const unsigned stack_free = lvgl_task_stack_free_bytes();
    ESP_LOGI(kTag,
             "updates=%llu flush/upd(ult/max)=%u/%u dur us(ult/p50/p95/max)=%lld/%lld/%lld/%lld",
             static_cast<unsigned long long>(m->updates()), m->last_flushes(), m->max_flushes(),
             static_cast<long long>(m->update_last_us()), static_cast<long long>(m->percentile_us(50)),
             static_cast<long long>(m->percentile_us(95)), static_cast<long long>(m->update_max_us()));
    ESP_LOGI(kTag,
             "espera flush us(ult/max)=%lld/%lld  lvgl_task pilha livre=%s%u B  "
             "heap int=%u KB  PSRAM livre=%u KB",
             static_cast<long long>(m->wait_last_us()), static_cast<long long>(m->wait_max_us()),
             stack_free == 0 ? "n/d " : "", stack_free,
             static_cast<unsigned>(heap_caps_get_free_size(MALLOC_CAP_INTERNAL) / 1024),
             static_cast<unsigned>(heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1024));
}
}  // namespace

void attach_render_probe(RenderMetrics& metrics, uint32_t dump_period_ms) {
    g_ctx.metrics_ = &metrics;
    lv_display_t* disp = lv_display_get_default();
    if (disp == nullptr) {
        ESP_LOGE(kTag, "sem display default — sonda nao anexada");
        return;
    }
    lv_display_add_event_cb(disp, on_render_event, LV_EVENT_RENDER_START, nullptr);
    lv_display_add_event_cb(disp, on_render_event, LV_EVENT_RENDER_READY, nullptr);
    lv_display_add_event_cb(disp, on_render_event, LV_EVENT_FLUSH_START, nullptr);
    lv_display_add_event_cb(disp, on_render_event, LV_EVENT_FLUSH_WAIT_START, nullptr);
    lv_display_add_event_cb(disp, on_render_event, LV_EVENT_FLUSH_WAIT_FINISH, nullptr);
    lv_timer_create(dump_cb, dump_period_ms, &metrics);
}

bool first_frame_rendered() {
    return g_ctx.first_frame_.load(std::memory_order_acquire);
}

}  // namespace diag
}  // namespace nova
