#include <cstdio>

#include "cache/weather_cache.hpp"

namespace {
int g_fail = 0;

void check(bool condition, const char* message) {
    if (!condition) {
        std::printf("  FAIL: %s\n", message);
        ++g_fail;
    }
}

nova::cache::WeatherCacheEntry sample() {
    nova::cache::WeatherCacheEntry entry{};
    entry.weather_.temperature_deci_c_ = 246;
    entry.weather_.apparent_temperature_deci_c_ = 231;
    entry.weather_.wind_speed_deci_kmh_ = 124;
    entry.weather_.weather_code_ = 3;
    entry.weather_.is_day_ = true;
    entry.weather_.source_ = nova::models::WeatherSource::kLive;
    entry.weather_.valid_ = true;
    entry.weather_.stale_ = false;
    entry.saved_utc_s_ = 1780000000ULL;
    return entry;
}

void test_round_trip() {
    uint8_t blob[nova::cache::WeatherCacheCodec::kBlobSize] = {};
    const auto entry = sample();
    check(nova::cache::WeatherCacheCodec::encode(entry, blob, sizeof(blob)) == nova::utils::Status::kOk,
          "encode aceita leitura live valida");
    const auto decoded = nova::cache::WeatherCacheCodec::decode(blob, sizeof(blob));
    check(decoded.is_ok(), "decode aceita blob valido");
    if (!decoded.is_ok()) return;
    check(decoded.value().saved_utc_s_ == entry.saved_utc_s_, "timestamp preservado");
    check(decoded.value().weather_.temperature_deci_c_ == 246, "temperatura preservada");
    check(decoded.value().weather_.source_ == nova::models::WeatherSource::kCache &&
              decoded.value().weather_.stale_,
          "leitura sempre vira cache stale");
}

void test_rejects_corruption() {
    uint8_t blob[nova::cache::WeatherCacheCodec::kBlobSize] = {};
    nova::cache::WeatherCacheCodec::encode(sample(), blob, sizeof(blob));
    blob[20] ^= 0x01U;
    const auto corrupt = nova::cache::WeatherCacheCodec::decode(blob, sizeof(blob));
    check(!corrupt.is_ok() && corrupt.status() == nova::utils::Status::kMalformed,
          "CRC rejeita corrupcao");
    const auto truncated = nova::cache::WeatherCacheCodec::decode(blob, sizeof(blob) - 1);
    check(!truncated.is_ok() && truncated.status() == nova::utils::Status::kMalformed,
          "tamanho truncado rejeitado");
}

void test_throttle() {
    using nova::cache::should_save_weather_cache;
    const uint64_t saved = 1780000000ULL;
    check(!should_save_weather_cache(saved, saved + 1799), "nao grava antes de 30 min");
    check(should_save_weather_cache(saved, saved + 1800), "grava exatamente aos 30 min");
    check(!should_save_weather_cache(saved, saved - 1), "relogio regressivo nao fura throttle");
}
}  // namespace

int main() {
    std::printf("weather cache tests:\n");
    test_round_trip();
    test_rejects_corruption();
    test_throttle();
    if (g_fail == 0) {
        std::printf("  PASS (todos)\n");
        return 0;
    }
    return 1;
}
