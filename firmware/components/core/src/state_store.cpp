#include "core/state_store.hpp"

namespace nova {
namespace core {

bool StateStore::set_clock(uint8_t hour, uint8_t minute, bool valid) {
    LockGuard g(lock_);
    models::ClockState& c = state_.clock_;
    // Dedup na origem: sem mudança, sem fato registrado, sem repintura.
    if (c.hour_ == hour && c.minute_ == minute && c.valid_ == valid) {
        return false;
    }
    c.hour_ = hour;
    c.minute_ = minute;
    c.valid_ = valid;
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

}  // namespace core
}  // namespace nova
