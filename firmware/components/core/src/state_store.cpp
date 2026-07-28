#include "core/state_store.hpp"

namespace nova {
namespace core {

bool StateStore::set_clock(models::ClockState clock) {
    LockGuard g(lock_);
    models::ClockState& c = state_.clock_;
    // Dedup na origem: sem mudança, sem fato registrado, sem repintura.
    if (c.last_update_ms_ == clock.last_update_ms_ && c.hour_ == clock.hour_ &&
        c.minute_ == clock.minute_ && c.source_ == clock.source_ && c.valid_ == clock.valid_ &&
        c.stale_ == clock.stale_) {
        return false;
    }
    c = clock;
    // Mutação e registro do fato acontecem sob o MESMO lock: um leitor nunca vê
    // estado novo sem o bit correspondente, nem o bit sem o estado.
    pending_ |= models::mask_of(models::Event::kClockChanged);
    return true;
}

bool StateStore::set_network(models::NetworkState s) {
    LockGuard g(lock_);
    if (state_.network_ == s) {
        return false;
    }
    state_.network_ = s;
    pending_ |= models::mask_of(models::Event::kNetworkChanged);
    return true;
}

bool StateStore::set_wifi_setup(models::WifiSetupState setup) {
    LockGuard g(lock_);
    models::WifiSetupState& current = state_.wifi_setup_;
    if (current.last_change_ms_ == setup.last_change_ms_ && current.phase_ == setup.phase_ &&
        current.has_saved_credentials_ == setup.has_saved_credentials_) {
        return false;
    }
    current = setup;
    pending_ |= models::mask_of(models::Event::kWifiSetupChanged);
    return true;
}

bool StateStore::set_weather(models::WeatherState weather) {
    LockGuard g(lock_);
    models::WeatherState& current = state_.weather_;
    if (current.last_update_ms_ == weather.last_update_ms_ &&
        current.temperature_deci_c_ == weather.temperature_deci_c_ &&
        current.apparent_temperature_deci_c_ == weather.apparent_temperature_deci_c_ &&
        current.wind_speed_deci_kmh_ == weather.wind_speed_deci_kmh_ &&
        current.weather_code_ == weather.weather_code_ && current.source_ == weather.source_ &&
        current.is_day_ == weather.is_day_ && current.valid_ == weather.valid_ &&
        current.stale_ == weather.stale_) {
        return false;
    }
    current = weather;
    pending_ |= models::mask_of(models::Event::kWeatherChanged);
    return true;
}

models::EventMask StateStore::take_pending_events() {
    LockGuard g(lock_);
    const models::EventMask m = pending_;
    pending_ = 0;
    return m;
}

models::ClockState StateStore::clock() const {
    LockGuard g(lock_);
    return state_.clock_;  // cópia por valor: o leitor nunca recebe referência
}

models::NetworkState StateStore::network() const {
    LockGuard g(lock_);
    return state_.network_;
}

models::WifiSetupState StateStore::wifi_setup() const {
    LockGuard g(lock_);
    return state_.wifi_setup_;
}

models::WeatherState StateStore::weather() const {
    LockGuard g(lock_);
    return state_.weather_;
}

}  // namespace core
}  // namespace nova
