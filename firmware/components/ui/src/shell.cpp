#include "ui/shell.hpp"

#include "lvgl.h"

namespace nova {
namespace ui {

namespace {
constexpr uint32_t kShellPeriodMs = 100;
constexpr size_t kNoScreen = ScreenRegistry::kMaxScreens;
}  // namespace

Shell::Shell(ScreenRegistry& registry) : registry_(registry) {}

bool Shell::attach(core::UiDispatcher& dispatcher) {
    if (dispatcher_ != nullptr) {
        return dispatcher_ == &dispatcher;
    }
    if (dispatcher.target_count() > core::UiDispatcher::kMaxTargets - registry_.count()) {
        return false;
    }
    dispatcher_ = &dispatcher;
    for (size_t i = 0; i < registry_.count(); ++i) {
        slots_[i].spec_ = registry_.at(i);
        slots_[i].dispatcher_id_ = dispatcher.register_target(
            slots_[i].spec_->invalidation_mask_, mark_dirty, &slots_[i]);
        if (slots_[i].dispatcher_id_ == core::UiDispatcher::kInvalidTarget) {
            return false;
        }
        dispatcher.set_visible(slots_[i].dispatcher_id_, false);
    }
    registry_.seal();
    return true;
}

bool Shell::start() {
    if (timer_ != nullptr || registry_.count() == 0) {
        return true;
    }
    timer_ = lv_timer_create(timer_cb, kShellPeriodMs, this);
    return timer_ != nullptr;
}

bool Shell::select(ScreenId id) {
    for (size_t i = 0; i < registry_.count(); ++i) {
        if (slots_[i].spec_->id_ == id) {
            return select_index(i);
        }
    }
    return false;
}

void Shell::mark_dirty(void* context) {
    static_cast<Slot*>(context)->dirty_.store(true);
}

void Shell::timer_cb(lv_timer_t* timer) {
    static_cast<Shell*>(lv_timer_get_user_data(timer))->render_pending();
}

bool Shell::select_index(size_t index) {
    if (dispatcher_ == nullptr || index >= registry_.count()) {
        return false;
    }
    if (visible_index_ < registry_.count()) {
        dispatcher_->set_visible(slots_[visible_index_].dispatcher_id_, false);
    }
    dispatcher_->set_visible(slots_[index].dispatcher_id_, true);
    visible_index_ = index;
    requested_index_.store(index);
    return true;
}

void Shell::render_pending() {
    const size_t requested = requested_index_.load();
    if (requested != active_index_ && requested < registry_.count()) {
        if (active_index_ < registry_.count() && slots_[active_index_].root_ != nullptr) {
            lv_obj_add_flag(slots_[active_index_].root_, LV_OBJ_FLAG_HIDDEN);
            if (slots_[active_index_].spec_->on_leave_ != nullptr) {
                slots_[active_index_].spec_->on_leave_(slots_[active_index_].spec_->context_);
            }
        }
        active_index_ = requested;
        Slot& slot = slots_[active_index_];
        if (slot.root_ == nullptr) {
            slot.root_ = slot.spec_->build_(lv_screen_active(), slot.spec_->context_);
        }
        if (slot.root_ != nullptr) {
            lv_obj_remove_flag(slot.root_, LV_OBJ_FLAG_HIDDEN);
            if (slot.spec_->on_enter_ != nullptr) {
                slot.spec_->on_enter_(slot.spec_->context_);
            }
        }
    }
    if (active_index_ < registry_.count() && slots_[active_index_].root_ != nullptr &&
        slots_[active_index_].dirty_.exchange(false)) {
        slots_[active_index_].spec_->update_(slots_[active_index_].spec_->context_);
    }
}

}  // namespace ui
}  // namespace nova
