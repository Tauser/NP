// Registro de telas: um spec por tela, sem condição no main (UI-PATTERN §2).
//
// PROPRIEDADE: a `app_loop` registra tudo no boot e chama `seal()` antes de o
// Shell criar alvos no UiDispatcher. Depois disso só há leitura pela UI/task.
#pragma once

#include <cstddef>
#include <cstdint>

#include "models/events.hpp"

struct _lv_obj_t;

namespace nova {
namespace ui {

using ScreenId = uint8_t;
constexpr ScreenId kInvalidScreenId = UINT8_MAX;
using ScreenBuildFn = _lv_obj_t* (*)(_lv_obj_t* parent, void* context);
using ScreenUpdateFn = void (*)(void* context);
using ScreenLifecycleFn = void (*)(void* context);

struct ScreenSpec {
    ScreenId id_ = kInvalidScreenId;
    models::EventMask invalidation_mask_ = 0;
    ScreenBuildFn build_ = nullptr;
    ScreenUpdateFn update_ = nullptr;
    ScreenLifecycleFn on_enter_ = nullptr;
    ScreenLifecycleFn on_leave_ = nullptr;
    void* context_ = nullptr;
};

class ScreenRegistry {
public:
    static constexpr size_t kMaxScreens = 8;

    bool register_screen(const ScreenSpec& spec);
    void seal() { sealed_ = true; }
    const ScreenSpec* at(size_t index) const;
    size_t count() const { return count_; }
    bool sealed() const { return sealed_; }

private:
    ScreenSpec specs_[kMaxScreens] = {};
    size_t count_ = 0;
    bool sealed_ = false;
};

}  // namespace ui
}  // namespace nova
