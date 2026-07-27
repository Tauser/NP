#include "core/action_queue.hpp"

namespace nova {
namespace core {

bool ActionQueue::push(Action a) {
    LockGuard g(lock_);
    if (count_ >= kCapacity) {
        ++overflows_;  // contado, nunca silencioso (ADR-008)
        return false;
    }
    buf_[(head_ + count_) % kCapacity] = a;
    ++count_;
    ++pushed_;
    return true;
}

bool ActionQueue::pop(Action& out) {
    LockGuard g(lock_);
    if (count_ == 0) {
        return false;
    }
    out = buf_[head_];
    head_ = (head_ + 1) % kCapacity;
    --count_;
    return true;
}

size_t ActionQueue::size() const {
    LockGuard g(lock_);
    return count_;
}

uint32_t ActionQueue::overflows() const {
    LockGuard g(lock_);
    return overflows_;
}

uint32_t ActionQueue::pushed() const {
    LockGuard g(lock_);
    return pushed_;
}

}  // namespace core
}  // namespace nova
