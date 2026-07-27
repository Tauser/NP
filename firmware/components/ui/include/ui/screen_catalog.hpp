// Catálogo único: cada tela futura adiciona apenas seu próprio ScreenSpec aqui,
// sem editar main/ ou o Shell (UI-PATTERN §2).
#pragma once

#include "ui/screen_registry.hpp"

namespace nova {
namespace ui {

void register_screens(ScreenRegistry& registry);

}  // namespace ui
}  // namespace nova
