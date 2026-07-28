#include "providers/open_meteo_weather_provider.hpp"

#include <cstddef>
#include <cstdint>
#include <cstring>

namespace nova {
namespace providers {
namespace {

constexpr char kBrasiliaCurrentUrl[] =
    "https://api.open-meteo.com/v1/forecast?latitude=-15.793889&longitude=-47.882778"
    "&current=temperature_2m,apparent_temperature,weather_code,is_day,wind_speed_10m"
    "&temperature_unit=celsius&wind_speed_unit=kmh&timezone=America%2FSao_Paulo";
constexpr uint32_t kTimeoutMs = 15000;

const char* find_key(const char* first, const char* last, const char* key) {
    const size_t key_size = std::strlen(key);
    for (const char* p = first; p + key_size <= last; ++p) {
        if (std::memcmp(p, key, key_size) == 0) {
            return p + key_size;
        }
    }
    return nullptr;
}

const char* after_colon(const char* p, const char* last) {
    while (p < last && (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')) ++p;
    if (p == last || *p != ':') return nullptr;
    ++p;
    while (p < last && (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')) ++p;
    return p;
}

bool parse_deci(const char* p, const char* last, int16_t& out) {
    bool neg = false;
    if (p < last && *p == '-') { neg = true; ++p; }
    if (p == last || *p < '0' || *p > '9') return false;
    int32_t whole = 0;
    while (p < last && *p >= '0' && *p <= '9') {
        whole = whole * 10 + (*p++ - '0');
        if (whole > 3276) return false;
    }
    int32_t deci = whole * 10;
    if (p < last && *p == '.') {
        ++p;
        if (p < last && *p >= '0' && *p <= '9') deci += *p - '0';
    }
    if (deci > 32767) return false;
    out = static_cast<int16_t>(neg ? -deci : deci);
    return true;
}

bool parse_uint(const char* p, const char* last, uint16_t& out) {
    if (p == last || *p < '0' || *p > '9') return false;
    uint32_t value = 0;
    while (p < last && *p >= '0' && *p <= '9') {
        value = value * 10U + static_cast<uint32_t>(*p++ - '0');
        if (value > UINT16_MAX) return false;
    }
    out = static_cast<uint16_t>(value);
    return true;
}

const char* object_end(const char* first, const char* last) {
    if (first == nullptr || first == last || *first != '{') return nullptr;
    uint8_t depth = 0;
    for (const char* p = first; p < last; ++p) {
        if (*p == '{') ++depth;
        if (*p == '}' && --depth == 0) return p + 1;
    }
    return nullptr;
}

const char* value_for(const char* first, const char* last, const char* key) {
    const char* found = find_key(first, last, key);
    return found == nullptr ? nullptr : after_colon(found, last);
}

}  // namespace

utils::Result<models::WeatherState> OpenMeteoWeatherProvider::fetch_current(
    utils::IHttpClient& client, utils::BoundedHttpBody& body) {
    const auto response = client.get(utils::HttpRequest{kBrasiliaCurrentUrl, kTimeoutMs}, body);
    if (!response.is_ok()) return utils::Result<models::WeatherState>::fail(response.status());
    if (response.value().status_code < 200 || response.value().status_code >= 300) {
        return utils::Result<models::WeatherState>::fail(utils::Status::kHttpError);
    }
    return parse_current(response.value().body, response.value().body_size);
}

utils::Result<models::WeatherState> OpenMeteoWeatherProvider::parse_current(const uint8_t* body,
                                                                               size_t size) {
    if (body == nullptr || size == 0) return utils::Result<models::WeatherState>::fail(utils::Status::kMalformed);
    const char* const first = reinterpret_cast<const char*>(body);
    const char* const last = first + size;
    const char* current = find_key(first, last, "\"current\"");
    current = current == nullptr ? nullptr : after_colon(current, last);
    const char* const end = object_end(current, last);
    if (end == nullptr) return utils::Result<models::WeatherState>::fail(utils::Status::kMalformed);

    const char* temperature = value_for(current, end, "\"temperature_2m\"");
    const char* apparent = value_for(current, end, "\"apparent_temperature\"");
    const char* code = value_for(current, end, "\"weather_code\"");
    const char* is_day = value_for(current, end, "\"is_day\"");
    const char* wind = value_for(current, end, "\"wind_speed_10m\"");
    if (temperature == nullptr || apparent == nullptr || code == nullptr || is_day == nullptr || wind == nullptr) {
        return utils::Result<models::WeatherState>::fail(utils::Status::kMalformed);
    }

    models::WeatherState weather{};
    uint16_t raw_code = 0;
    uint16_t raw_day = 0;
    int16_t wind_deci = 0;
    if (!parse_deci(temperature, end, weather.temperature_deci_c_) ||
        !parse_deci(apparent, end, weather.apparent_temperature_deci_c_) ||
        !parse_deci(wind, end, wind_deci) || wind_deci < 0 || !parse_uint(code, end, raw_code) ||
        raw_code > UINT8_MAX || !parse_uint(is_day, end, raw_day) || raw_day > 1) {
        return utils::Result<models::WeatherState>::fail(utils::Status::kMalformed);
    }
    weather.wind_speed_deci_kmh_ = static_cast<uint16_t>(wind_deci);
    weather.weather_code_ = static_cast<uint8_t>(raw_code);
    weather.is_day_ = raw_day == 1;
    weather.source_ = models::WeatherSource::kLive;
    weather.valid_ = true;
    weather.stale_ = false;
    return utils::Result<models::WeatherState>::ok(weather);
}

}  // namespace providers
}  // namespace nova
