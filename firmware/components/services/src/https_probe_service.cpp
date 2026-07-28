#include "services/https_probe_service.hpp"

#include "esp_heap_caps.h"
#include "esp_log.h"

namespace nova {
namespace services {
namespace {
constexpr const char* kTag = "https.probe";
constexpr char kProbeUrl[] = "https://example.com/";
constexpr uint8_t kSampleCount = 3;
constexpr core::RequestPolicy kProbePolicy{3000, 5000, 60U * 1000U, 1, 0, 0};
}  // namespace

HttpsProbeService::HttpsProbeService(core::StateStore& store, board::IBoard& board,
                                     NetworkWorker& worker)
    : store_(store), board_(board), worker_(worker) {}

utils::Status HttpsProbeService::start() {
    const auto id = worker_.register_handler(kProbePolicy, *this);
    if (!id.is_ok()) {
        return id.status();
    }
    request_id_ = id.value();
    return utils::Status::kOk;
}

void HttpsProbeService::tick(uint64_t now_ms) {
    (void)now_ms;
    if (finished_ || request_id_ == core::kInvalidRequestId ||
        board_.wifi_connection_state() != board::WifiConnectionState::kConnected ||
        store_.clock().source_ != models::ClockSource::kNtp) {
        return;
    }
    if (!enabled_) {
        enabled_ = worker_.set_enabled(request_id_, true);
        if (enabled_) {
            ESP_LOGI(kTag, "3 sondas HTTPS habilitadas apos NTP");
        }
        return;
    }
    if (completed_.load() >= kSampleCount && worker_.set_enabled(request_id_, false)) {
        finished_ = true;
        ESP_LOGI(kTag, "3/3 sondas concluidas; HTTPS desabilitado ate o proximo boot");
    }
}

utils::Status HttpsProbeService::execute(core::RequestLease lease, utils::IHttpClient& client,
                                         utils::BoundedHttpBody& body) {
    (void)lease;
    const uint8_t sample = static_cast<uint8_t>(completed_.load() + 1);
    const size_t before = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
    const auto response = client.get(utils::HttpRequest{kProbeUrl, 15000}, body);
    const size_t after = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
    if (!response.is_ok()) {
        ESP_LOGW(kTag, "HTTPS %u/%u falhou: %s; heap antes/depois=%u/%u KiB", sample,
                 kSampleCount, utils::to_string(response.status()), static_cast<unsigned>(before / 1024),
                 static_cast<unsigned>(after / 1024));
        completed_.fetch_add(1);
        return response.status();
    }
    ESP_LOGI(kTag, "HTTPS %u/%u ok: status=%u corpo=%u B heap antes/depois=%u/%u KiB", sample,
             kSampleCount, static_cast<unsigned>(response.value().status_code),
             static_cast<unsigned>(response.value().body_size), static_cast<unsigned>(before / 1024),
             static_cast<unsigned>(after / 1024));
    completed_.fetch_add(1);
    return utils::Status::kOk;
}

}  // namespace services
}  // namespace nova
