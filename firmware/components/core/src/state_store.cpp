#include "core/state_store.hpp"

namespace nova {
namespace core {

bool StateStore::set_clock(uint8_t hour, uint8_t minute, bool valid) {
    LockGuard g(lock_);
    models::ClockState& c = state_.clock_;
    if (c.hour_ == hour && c.minute_ == minute && c.valid_ == valid) {
        return false;  // dedup na origem: sem mudança, sem evento, sem repintura
    }
    c.hour_ = hour;
    c.minute_ = minute;
    c.valid_ = valid;
    return true;
}

bool StateStore::set_network(models::NetworkState s) {
    LockGuard g(lock_);
    if (state_.network_ == s) {
        return false;
    }
    state_.network_ = s;
    return true;
}

models::ClockState StateStore::clock() const {
    LockGuard g(lock_);
    return state_.clock_;
}

models::NetworkState StateStore::network() const {
    LockGuard g(lock_);
    return state_.network_;
}

}  // namespace core
}  // namespace nova
