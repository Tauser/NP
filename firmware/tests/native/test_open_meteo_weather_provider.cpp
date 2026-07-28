#include <cstdio>
#include <fstream>
#include <string>

#include "providers/open_meteo_weather_provider.hpp"

namespace {
int g_fail = 0;

void check(bool condition, const char* message) {
    if (!condition) {
        std::printf("  FAIL: %s\n", message);
        ++g_fail;
    }
}

std::string fixture(const char* name) {
    std::ifstream file(name, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
}

void test_valid_fixture() {
    const std::string json = fixture("firmware/tests/fixtures/open_meteo_brasilia_current.json");
    const auto parsed = nova::providers::OpenMeteoWeatherProvider::parse_current(
        reinterpret_cast<const uint8_t*>(json.data()), json.size());
    check(parsed.is_ok(), "fixture real faz parse");
    if (!parsed.is_ok()) return;
    const auto& weather = parsed.value();
    check(weather.temperature_deci_c_ == 246, "temperatura em decimos");
    check(weather.apparent_temperature_deci_c_ == 231, "sensacao em decimos");
    check(weather.wind_speed_deci_kmh_ == 124, "vento em decimos");
    check(weather.weather_code_ == 3 && weather.is_day_, "codigo e periodo corretos");
    check(weather.valid_ && !weather.stale_, "leitura live valida e fresca");
}

void test_invalid_fixtures() {
    const std::string malformed = fixture("firmware/tests/fixtures/open_meteo_malformed.json");
    const auto missing = nova::providers::OpenMeteoWeatherProvider::parse_current(
        reinterpret_cast<const uint8_t*>(malformed.data()), malformed.size());
    check(!missing.is_ok() && missing.status() == nova::utils::Status::kMalformed,
          "fixture malformada e rejeitada");

    const std::string truncated = fixture("firmware/tests/fixtures/open_meteo_truncated.json");
    const auto cut = nova::providers::OpenMeteoWeatherProvider::parse_current(
        reinterpret_cast<const uint8_t*>(truncated.data()), truncated.size());
    check(!cut.is_ok() && cut.status() == nova::utils::Status::kMalformed,
          "fixture truncada e rejeitada");
}
}  // namespace

int main() {
    std::printf("open_meteo_weather_provider tests:\n");
    test_valid_fixture();
    test_invalid_fixtures();
    if (g_fail == 0) {
        std::printf("  PASS (todos)\n");
        return 0;
    }
    return 1;
}
