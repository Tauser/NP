// Shell: ponte entre invalidação pura e widgets LVGL (UI-PATTERN §3/§5).
#pragma once

#include <atomic>
#include <cstddef>

#include "core/ui_dispatcher.hpp"
#include "ui/screen_registry.hpp"

struct _lv_timer_t;

namespace nova {
namespace ui {

class Shell {
public:
    explicit Shell(ScreenRegistry& registry);

    // Chamado pela app_loop antes de tasks de UI; apenas registra callbacks
    // puros no dispatcher. O registry é selado neste ponto.
    bool attach(core::UiDispatcher& dispatcher);

    // Chamado sob o lock semântico do board. Cria um timer LVGL que executa
    // build/update dentro da lvgl_task, nunca no callback do dispatcher.
    bool start();
    bool select(ScreenId id);

private:
    struct Slot {
        const ScreenSpec* spec_ = nullptr;
        size_t dispatcher_id_ = core::UiDispatcher::kInvalidTarget;
        _lv_obj_t* root_ = nullptr;
        std::atomic<bool> dirty_{false};
    };

    static void mark_dirty(void* context);
    static void timer_cb(_lv_timer_t* timer);
    void render_pending();
    bool select_index(size_t index);

    ScreenRegistry& registry_;
    core::UiDispatcher* dispatcher_ = nullptr;
    Slot slots_[ScreenRegistry::kMaxScreens] = {};
    _lv_timer_t* timer_ = nullptr;
    std::atomic<size_t> requested_index_{ScreenRegistry::kMaxScreens};
    size_t visible_index_ = ScreenRegistry::kMaxScreens;  // dona: app_loop
    size_t active_index_ = ScreenRegistry::kMaxScreens;   // dona: lvgl_task
};

}  // namespace ui
}  // namespace nova
