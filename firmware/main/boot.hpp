// Declaração pura da sequência de boot. app_main.cpp inclui SÓ este header (e
// esp_log), para continuar compilável no host_check (shims). Toda a fiação real
// com IDF/LVGL/board vive em boot.cpp, que o host_check não compila.
#pragma once

namespace nova {
namespace app {

void run();

}  // namespace app
}  // namespace nova
