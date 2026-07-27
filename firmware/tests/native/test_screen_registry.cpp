// Registro de telas sem LVGL: garante crescimento por spec, não por if/else.
#include <cstdio>

#include "ui/screen_registry.hpp"

namespace {
int g_fail = 0;

void check(bool condition, const char* what) {
    if (!condition) {
        std::printf("  FAIL: %s\n", what);
        ++g_fail;
    }
}

_lv_obj_t* build(_lv_obj_t*, void*) { return nullptr; }
void update(void*) {}

nova::ui::ScreenSpec spec(nova::ui::ScreenId id) {
    nova::ui::ScreenSpec value;
    value.id_ = id;
    value.invalidation_mask_ = nova::models::mask_of(nova::models::Event::kClockChanged);
    value.build_ = build;
    value.update_ = update;
    return value;
}

void test_registry() {
    nova::ui::ScreenRegistry registry;
    check(registry.register_screen(spec(1)), "aceita spec valida");
    check(!registry.register_screen(spec(1)), "recusa id duplicado");
    check(registry.count() == 1 && registry.at(0)->id_ == 1, "mantem spec registrado");
    check(registry.at(1) == nullptr, "indice fora da faixa e nulo");
    for (nova::ui::ScreenId id = 2; id <= nova::ui::ScreenRegistry::kMaxScreens; ++id) {
        check(registry.register_screen(spec(id)), "aceita ate a capacidade");
    }
    check(!registry.register_screen(spec(9)), "recusa acima da capacidade");
}

void test_invalid_and_sealed() {
    nova::ui::ScreenRegistry registry;
    nova::ui::ScreenSpec invalid = spec(nova::ui::kInvalidScreenId);
    check(!registry.register_screen(invalid), "recusa id invalido");
    invalid = spec(1);
    invalid.update_ = nullptr;
    check(!registry.register_screen(invalid), "recusa update ausente");
    registry.seal();
    check(!registry.register_screen(spec(2)), "recusa registro apos selar");
}
}  // namespace

int main() {
    std::printf("screen registry tests:\n");
    test_registry();
    test_invalid_and_sealed();
    if (g_fail == 0) {
        std::printf("  PASS (todos)\n");
        return 0;
    }
    std::printf("  %d checagens falharam\n", g_fail);
    return 1;
}
