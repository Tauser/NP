#include "core/request_orchestrator.hpp"

#include <limits>

namespace nova {
namespace core {

RequestOrchestrator::RequestOrchestrator(ILock& lock, uint32_t global_gap_ms,
                                         JitterFn jitter, void* jitter_ctx)
    : lock_(lock),
      global_gap_ms_(global_gap_ms),
      jitter_(jitter == nullptr ? no_jitter : jitter),
      jitter_ctx_(jitter_ctx) {}

utils::Result<RequestId> RequestOrchestrator::register_request(RequestPolicy policy) {
    if (!is_valid_policy(policy)) {
        return utils::Result<RequestId>::fail(utils::Status::kInvalidArg);
    }

    LockGuard guard(lock_);
    for (size_t i = 0; i < kMaxRequests; ++i) {
        if (!slots_[i].registered) {
            Slot& slot = slots_[i];
            slot.policy = policy;
            slot.registered = true;
            return utils::Result<RequestId>::ok(static_cast<RequestId>(i));
        }
    }
    return utils::Result<RequestId>::fail(utils::Status::kNoMemory);
}

bool RequestOrchestrator::set_enabled(RequestId id, bool enabled) {
    LockGuard guard(lock_);
    if (!is_valid_id(id)) {
        return false;
    }

    Slot& slot = slots_[id];
    slot.enabled = enabled;
    return true;
}

utils::Result<RequestLease> RequestOrchestrator::take_next(uint64_t now_ms) {
    LockGuard guard(lock_);
    if (active_id_ != kInvalidRequestId || now_ms < next_global_start_ms_) {
        return utils::Result<RequestLease>::fail(utils::Status::kBusy);
    }

    RequestId candidate = kInvalidRequestId;
    for (size_t i = 0; i < kMaxRequests; ++i) {
        Slot& slot = slots_[i];
        if (!slot.registered || !slot.enabled || now_ms < slot.next_due_ms) {
            continue;
        }
        if (candidate == kInvalidRequestId || slot.policy.priority < slots_[candidate].policy.priority) {
            candidate = static_cast<RequestId>(i);
        }
    }
    if (candidate == kInvalidRequestId) {
        return utils::Result<RequestLease>::fail(utils::Status::kBusy);
    }

    Slot& slot = slots_[candidate];
    if (slot.circuit == CircuitState::kOpen) {
        slot.circuit = CircuitState::kHalfOpen;
    }
    ++slot.sequence;
    active_id_ = candidate;
    active_sequence_ = slot.sequence;
    next_global_start_ms_ = after(now_ms, global_gap_ms_);
    return utils::Result<RequestLease>::ok(RequestLease{candidate, slot.sequence});
}

bool RequestOrchestrator::complete(RequestLease lease, utils::Status result, uint64_t now_ms) {
    LockGuard guard(lock_);
    if (!is_valid_id(lease.id) || lease.id != active_id_ || lease.sequence != active_sequence_) {
        return false;
    }

    Slot& slot = slots_[lease.id];
    active_id_ = kInvalidRequestId;
    active_sequence_ = 0;

    if (result == utils::Status::kOk) {
        slot.failures = 0;
        slot.circuit = CircuitState::kClosed;
        slot.next_due_ms = after(now_ms, slot.policy.min_interval_ms);
        return true;
    }

    // Payload inválido ou pedido malformado não merece a sequência de retries
    // curtos: abre logo o breaker e só permite nova probe depois do cooldown
    // máximo. Assim o provider não é martelado por uma falha que ele não pode
    // resolver sem mudar payload ou configuração.
    if (!utils::is_transient(result)) {
        slot.failures = slot.policy.failures_to_open;
        slot.circuit = CircuitState::kOpen;
        slot.next_due_ms = after(
            now_ms, jittered_delay(slot.policy.max_backoff_ms, slot.policy.jitter_percent));
        return true;
    }

    if (slot.failures < std::numeric_limits<uint8_t>::max()) {
        ++slot.failures;
    }
    if (slot.failures >= slot.policy.failures_to_open) {
        slot.circuit = CircuitState::kOpen;
    }
    slot.next_due_ms = after(now_ms, jittered_delay(retry_delay(slot), slot.policy.jitter_percent));
    return true;
}

bool RequestOrchestrator::has_active_request() const {
    LockGuard guard(lock_);
    return active_id_ != kInvalidRequestId;
}

CircuitState RequestOrchestrator::circuit_state(RequestId id) const {
    LockGuard guard(lock_);
    return is_valid_id(id) ? slots_[id].circuit : CircuitState::kOpen;
}

uint32_t RequestOrchestrator::no_jitter(uint32_t upper_inclusive, void* ctx) {
    (void)ctx;
    return upper_inclusive / 2;
}

bool RequestOrchestrator::is_valid_policy(const RequestPolicy& policy) {
    return policy.min_interval_ms > 0 && policy.initial_backoff_ms > 0 &&
           policy.max_backoff_ms >= policy.initial_backoff_ms && policy.failures_to_open > 0 &&
           policy.jitter_percent <= 50;
}

uint64_t RequestOrchestrator::after(uint64_t now_ms, uint32_t delay_ms) {
    const uint64_t max = std::numeric_limits<uint64_t>::max();
    return now_ms > max - delay_ms ? max : now_ms + delay_ms;
}

uint32_t RequestOrchestrator::retry_delay(const Slot& slot) const {
    uint32_t delay = slot.policy.initial_backoff_ms;
    for (uint8_t i = 1; i < slot.failures && delay < slot.policy.max_backoff_ms; ++i) {
        delay = delay > slot.policy.max_backoff_ms / 2 ? slot.policy.max_backoff_ms : delay * 2;
    }
    return delay;
}

uint32_t RequestOrchestrator::jittered_delay(uint32_t delay_ms, uint8_t percent) const {
    const uint32_t spread =
        static_cast<uint32_t>(static_cast<uint64_t>(delay_ms) * percent / 100);
    if (spread == 0) {
        return delay_ms;
    }
    const uint32_t upper = spread * 2;
    const uint32_t sample = jitter_(upper, jitter_ctx_);
    return delay_ms - spread + (sample > upper ? upper : sample);
}

bool RequestOrchestrator::is_valid_id(RequestId id) const {
    return id < kMaxRequests && slots_[id].registered;
}

}  // namespace core
}  // namespace nova
