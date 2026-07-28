#include "services/sntp_service.hpp"

#include <ctime>

#include "esp_err.h"
#include "esp_log.h"
#include "esp_netif_sntp.h"
#include "freertos/FreeRTOS.h"

namespace nova {
namespace services {
namespace {
constexpr const char* kTag = "sntp";
constexpr char kServer[] = "pool.ntp.org";
}  // namespace

SntpService::SntpService(board::IBoard& board, ClockService& clock) : board_(board), clock_(clock) {}

utils::Status SntpService::start() { return utils::Status::kOk; }

void SntpService::tick(uint64_t now_ms) {
    if (synchronized_ || board_.wifi_connection_state() != board::WifiConnectionState::kConnected) {
        return;
    }
    if (!initialized_) {
        esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG(kServer);
        if (esp_netif_sntp_init(&config) != ESP_OK) {
            ESP_LOGW(kTag, "inicializacao SNTP falhou; mantendo hora offline");
            return;
        }
        initialized_ = true;
        ESP_LOGI(kTag, "aguardando UTC por SNTP");
        return;
    }
    if (esp_netif_sntp_sync_wait(0) != ESP_OK) {
        return;
    }
    const time_t unix_time = time(nullptr);
    if (unix_time <= 0 ||
        clock_.accept_ntp_time(models::UtcTime{static_cast<uint64_t>(unix_time)}, now_ms) != utils::Status::kOk) {
        ESP_LOGW(kTag, "UTC SNTP invalido; mantendo hora anterior");
        return;
    }
    synchronized_ = true;
    ESP_LOGI(kTag, "UTC sincronizado por SNTP");
}

}  // namespace services
}  // namespace nova
