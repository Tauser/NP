#include "services/weather_service.hpp"

#include <ctime>

#include "esp_log.h"

namespace nova {
namespace services {
namespace {
constexpr const char* kTag = "weather";
constexpr core::RequestPolicy kWeatherPolicy{
    30U * 60U * 1000U, 5000, 15U * 60U * 1000U, 3, 2, 10};
}  // namespace

WeatherService::WeatherService(core::StateStore& store, board::IBoard& board, NetworkWorker& worker,
                               providers::IWeatherProvider& provider, cache::IWeatherCache& cache)
    : store_(store), board_(board), worker_(worker), provider_(provider), cache_(cache) {}

utils::Status WeatherService::start() {
    load_cache();
    const auto id = worker_.register_handler(kWeatherPolicy, *this);
    if (!id.is_ok()) return id.status();
    request_id_ = id.value();
    return utils::Status::kOk;
}

void WeatherService::tick(uint64_t now_ms) {
    now_ms_.store(now_ms);
    if (cache_save_pending_.exchange(false)) {
        save_cache(static_cast<uint64_t>(std::time(nullptr)));
    }
    if (enabled_ || request_id_ == core::kInvalidRequestId ||
        board_.wifi_connection_state() != board::WifiConnectionState::kConnected ||
        store_.clock().source_ != models::ClockSource::kNtp) {
        return;
    }
    enabled_ = worker_.set_enabled(request_id_, true);
    if (enabled_) ESP_LOGI(kTag, "clima Brasilia/DF habilitado apos NTP; intervalo=30 min");
}

utils::Status WeatherService::execute(core::RequestLease lease, utils::IHttpClient& client,
                                      utils::BoundedHttpBody& body) {
    (void)lease;
    const auto weather = provider_.fetch_current(client, body);
    if (!weather.is_ok()) {
        mark_stale();
        ESP_LOGW(kTag, "consulta falhou: %s", utils::to_string(weather.status()));
        return weather.status();
    }
    models::WeatherState current = weather.value();
    current.last_update_ms_ = now_ms_.load();
    store_.set_weather(current);
    cache_save_pending_.store(true);
    ESP_LOGI(kTag, "atualizado: %d.%d C sensacao=%d.%d C vento=%u.%u km/h codigo=%u",
             current.temperature_deci_c_ / 10, current.temperature_deci_c_ < 0 ? -current.temperature_deci_c_ % 10 : current.temperature_deci_c_ % 10,
             current.apparent_temperature_deci_c_ / 10, current.apparent_temperature_deci_c_ < 0 ? -current.apparent_temperature_deci_c_ % 10 : current.apparent_temperature_deci_c_ % 10,
             static_cast<unsigned>(current.wind_speed_deci_kmh_ / 10),
             static_cast<unsigned>(current.wind_speed_deci_kmh_ % 10),
             static_cast<unsigned>(current.weather_code_));
    return utils::Status::kOk;
}

void WeatherService::load_cache() {
    const auto cached = cache_.load();
    if (!cached.is_ok()) {
        if (cached.status() != utils::Status::kNotFound) {
            ESP_LOGW(kTag, "cache ignorado: %s", utils::to_string(cached.status()));
        }
        return;
    }
    saved_cache_utc_s_ = cached.value().saved_utc_s_;
    store_.set_weather(cached.value().weather_);
    cache_loaded_ = true;
    ESP_LOGI(kTag, "cache offline carregado; dado marcado stale");
}

void WeatherService::save_cache(uint64_t now_utc_s) {
    if (now_utc_s < 1704067200ULL || !cache::should_save_weather_cache(saved_cache_utc_s_, now_utc_s)) {
        return;
    }
    const models::WeatherState weather = store_.weather();
    if (!weather.valid_ || weather.source_ != models::WeatherSource::kLive) return;
    const cache::WeatherCacheEntry entry{weather, now_utc_s};
    const utils::Status saved = cache_.save(entry);
    if (saved == utils::Status::kOk) {
        saved_cache_utc_s_ = now_utc_s;
        ESP_LOGI(kTag, "cache salvo; proxima escrita em >=30 min");
    } else {
        ESP_LOGW(kTag, "cache nao salvo: %s", utils::to_string(saved));
    }
}

void WeatherService::mark_stale() {
    models::WeatherState current = store_.weather();
    if (!current.valid_ || current.stale_) return;
    current.stale_ = true;
    store_.set_weather(current);
}

}  // namespace services
}  // namespace nova
