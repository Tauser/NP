#include "core/state_store.hpp"

namespace nova {
namespace core {

bool StateStore::set_clock(uint8_t hour, uint8_t minute, bool valid) {
    bool changed = false;
    {
        LockGuard g(lock_);
        models::ClockState& c = state_.clock_;
        // Dedup na origem: sem mudança, sem evento, sem repintura.
        if (c.hour_ != hour || c.minute_ != minute || c.valid_ != valid) {
            c.hour_ = hour;
            c.minute_ = minute;
            c.valid_ = valid;
            changed = true;
        }
    }  // lock LIBERADO aqui, antes de publicar (ver header: handler pode ler estado)
    if (changed && bus_ != nullptr) {
        bus_->publish(models::Event::kClockChanged);
    }
    return changed;
}

bool StateStore::set_network(models::NetworkState s) {
    bool changed = false;
    {
        LockGuard g(lock_);
        if (state_.network_ != s) {
            state_.network_ = s;
            changed = true;
        }
    }
    if (changed && bus_ != nullptr) {
        bus_->publish(models::Event::kNetworkChanged);
    }
    return changed;
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
