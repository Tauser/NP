#include "core/ui_dispatcher.hpp"

namespace nova {
namespace core {

size_t UiDispatcher::register_target(models::EventMask mask, InvalidateFn fn, void* ctx) {
    if (fn == nullptr || count_ >= kMaxTargets) {
        return kInvalidTarget;
    }
    Target& t = targets_[count_];
    t.mask_ = mask;
    t.fn_ = fn;
    t.ctx_ = ctx;
    t.visible_ = true;
    t.dirty_ = false;
    return count_++;
}

void UiDispatcher::on_event(models::Event e, void* self) {
    // Só acumula. Invalidar aqui chamaria update() uma vez POR EVENTO, que é
    // exatamente o custo que o coalescing existe para evitar.
    static_cast<UiDispatcher*>(self)->pending_ |= models::mask_of(e);
}

void UiDispatcher::invalidate(Target& t) {
    t.fn_(t.ctx_);
    t.dirty_ = false;
    ++invalidations_;
}

size_t UiDispatcher::dispatch() {
    // Passo 1: marca sujo quem se interessa por algum evento do ciclo.
    for (size_t i = 0; i < count_; ++i) {
        if ((targets_[i].mask_ & pending_) != 0) {
            targets_[i].dirty_ = true;
        }
    }
    pending_ = 0;

    // Passo 2: invalida os sujos VISÍVEIS — uma vez cada, independentemente de
    // quantos eventos do ciclo os tenham sujado.
    size_t n = 0;
    for (size_t i = 0; i < count_; ++i) {
        if (targets_[i].dirty_ && targets_[i].visible_) {
            invalidate(targets_[i]);
            ++n;
        }
    }
    return n;
}

void UiDispatcher::set_visible(size_t id, bool visible) {
    if (id >= count_) {
        return;
    }
    Target& t = targets_[id];
    const bool entering = visible && !t.visible_;
    t.visible_ = visible;
    // `on_enter`: quem ficou sujo enquanto invisível repinta ao entrar.
    if (entering && t.dirty_) {
        invalidate(t);
    }
}

bool UiDispatcher::is_dirty(size_t id) const {
    return id < count_ && targets_[id].dirty_;
}

}  // namespace core
}  // namespace nova
