// NovaPanel — ponto de entrada.
//
// ESTE ARQUIVO É WIRING (docs/ARCHITECTURE.md §3): delega a montagem do grafo
// para nova::app::run(). Não tem driver, regra de domínio nem política de
// repintura. Limite duro de 300 linhas, verificado por gate.
//
// Mantido propositalmente magro: inclui só esp_log e um header PURO (boot.hpp),
// para continuar compilável no host_check (que só tem shims). A fiação real com
// IDF/LVGL/board vive em boot.cpp.
#include "esp_log.h"

#include "boot.hpp"

namespace {
constexpr const char* kTag = "NovaPanel";
}  // namespace

extern "C" void app_main(void) {
    ESP_LOGI(kTag, "app_main -> nova::app::run()");
    nova::app::run();
}
