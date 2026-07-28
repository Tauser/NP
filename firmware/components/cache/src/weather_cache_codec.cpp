#include "cache/weather_cache.hpp"

namespace nova {
namespace cache {
namespace {
constexpr uint32_t kMagic = 0x4E505743U;  // "NPWC"
constexpr uint16_t kVersion = 1;
constexpr uint16_t kPayloadSize = 16;

void put_u16(uint8_t* p, uint16_t value) {
    p[0] = static_cast<uint8_t>(value);
    p[1] = static_cast<uint8_t>(value >> 8);
}

void put_u32(uint8_t* p, uint32_t value) {
    for (uint8_t i = 0; i < 4; ++i) p[i] = static_cast<uint8_t>(value >> (i * 8));
}

void put_u64(uint8_t* p, uint64_t value) {
    for (uint8_t i = 0; i < 8; ++i) p[i] = static_cast<uint8_t>(value >> (i * 8));
}

uint16_t get_u16(const uint8_t* p) {
    return static_cast<uint16_t>(p[0]) | static_cast<uint16_t>(p[1]) << 8;
}

uint32_t get_u32(const uint8_t* p) {
    uint32_t value = 0;
    for (uint8_t i = 0; i < 4; ++i) value |= static_cast<uint32_t>(p[i]) << (i * 8);
    return value;
}

uint64_t get_u64(const uint8_t* p) {
    uint64_t value = 0;
    for (uint8_t i = 0; i < 8; ++i) value |= static_cast<uint64_t>(p[i]) << (i * 8);
    return value;
}

uint32_t crc32(const uint8_t* data, size_t size) {
    uint32_t crc = 0xFFFFFFFFU;
    for (size_t i = 0; i < size; ++i) {
        crc ^= data[i];
        for (uint8_t bit = 0; bit < 8; ++bit) {
            crc = (crc >> 1) ^ ((crc & 1U) == 0 ? 0U : 0xEDB88320U);
        }
    }
    return ~crc;
}

bool valid_entry(const WeatherCacheEntry& entry) {
    return entry.weather_.valid_ && entry.saved_utc_s_ != 0 &&
           entry.weather_.weather_code_ <= 99 && entry.weather_.wind_speed_deci_kmh_ <= 3000;
}
}  // namespace

utils::Status WeatherCacheCodec::encode(const WeatherCacheEntry& entry, uint8_t* out, size_t capacity) {
    if (out == nullptr || capacity < kBlobSize || !valid_entry(entry)) return utils::Status::kInvalidArg;
    uint8_t* const payload = out + 12;
    put_u64(payload, entry.saved_utc_s_);
    put_u16(payload + 8, static_cast<uint16_t>(entry.weather_.temperature_deci_c_));
    put_u16(payload + 10, static_cast<uint16_t>(entry.weather_.apparent_temperature_deci_c_));
    put_u16(payload + 12, entry.weather_.wind_speed_deci_kmh_);
    payload[14] = entry.weather_.weather_code_;
    payload[15] = entry.weather_.is_day_ ? 1 : 0;
    put_u32(out, kMagic);
    put_u16(out + 4, kVersion);
    put_u16(out + 6, kPayloadSize);
    put_u32(out + 8, crc32(payload, kPayloadSize));
    return utils::Status::kOk;
}

utils::Result<WeatherCacheEntry> WeatherCacheCodec::decode(const uint8_t* data, size_t size) {
    if (data == nullptr || size != kBlobSize || get_u32(data) != kMagic ||
        get_u16(data + 4) != kVersion || get_u16(data + 6) != kPayloadSize) {
        return utils::Result<WeatherCacheEntry>::fail(utils::Status::kMalformed);
    }
    const uint8_t* const payload = data + 12;
    if (get_u32(data + 8) != crc32(payload, kPayloadSize)) {
        return utils::Result<WeatherCacheEntry>::fail(utils::Status::kMalformed);
    }
    WeatherCacheEntry entry{};
    entry.saved_utc_s_ = get_u64(payload);
    entry.weather_.temperature_deci_c_ = static_cast<int16_t>(get_u16(payload + 8));
    entry.weather_.apparent_temperature_deci_c_ = static_cast<int16_t>(get_u16(payload + 10));
    entry.weather_.wind_speed_deci_kmh_ = get_u16(payload + 12);
    entry.weather_.weather_code_ = payload[14];
    entry.weather_.is_day_ = payload[15] == 1;
    entry.weather_.source_ = models::WeatherSource::kCache;
    entry.weather_.valid_ = true;
    entry.weather_.stale_ = true;
    if (!valid_entry(entry) || payload[15] > 1) {
        return utils::Result<WeatherCacheEntry>::fail(utils::Status::kMalformed);
    }
    return utils::Result<WeatherCacheEntry>::ok(entry);
}

bool should_save_weather_cache(uint64_t saved_utc_s, uint64_t now_utc_s) {
    return now_utc_s >= saved_utc_s &&
           (saved_utc_s == 0 || now_utc_s - saved_utc_s >= kWeatherCacheMinWriteIntervalS);
}

}  // namespace cache
}  // namespace nova
