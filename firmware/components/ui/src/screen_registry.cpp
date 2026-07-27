#include "ui/screen_registry.hpp"

namespace nova {
namespace ui {

bool ScreenRegistry::register_screen(const ScreenSpec& spec) {
    if (sealed_ || spec.id_ == kInvalidScreenId || spec.build_ == nullptr || spec.update_ == nullptr ||
        count_ == kMaxScreens) {
        return false;
    }
    for (size_t i = 0; i < count_; ++i) {
        if (specs_[i].id_ == spec.id_) {
            return false;
        }
    }
    specs_[count_++] = spec;
    return true;
}

const ScreenSpec* ScreenRegistry::at(size_t index) const {
    return index < count_ ? &specs_[index] : nullptr;
}

}  // namespace ui
}  // namespace nova
