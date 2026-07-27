#include "ui/screen_catalog.hpp"

namespace nova {
namespace ui {

void register_screens(ScreenRegistry& registry) {
    // Intencionalmente vazio: ADR-023 proíbe criar tela sem provider + service
    // + estado aprovados. A primeira tela entra como uma linha aqui.
    (void)registry;
}

}  // namespace ui
}  // namespace nova
