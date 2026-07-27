#include "core/event_bus.hpp"

namespace nova {
namespace core {

bool EventBus::subscribe(EventHandler handler, void* ctx) {
    if (handler == nullptr || count_ >= kMaxSubscribers) {
        return false;
    }
    slots_[count_].handler_ = handler;
    slots_[count_].ctx_ = ctx;
    ++count_;
    return true;
}

size_t EventBus::publish(models::Event e) {
    // Reentrância: um handler que publica durante o despacho recursionaria.
    // Rejeitar e CONTAR é o comportamento; descartar em silêncio, não.
    if (dispatching_) {
        ++rejected_reentrant_;
        return 0;
    }
    dispatching_ = true;
    size_t called = 0;
    for (size_t i = 0; i < count_; ++i) {
        slots_[i].handler_(e, slots_[i].ctx_);
        ++called;
    }
    dispatching_ = false;
    ++published_;
    return called;
}

}  // namespace core
}  // namespace nova
