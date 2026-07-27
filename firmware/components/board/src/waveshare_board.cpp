// WaveshareBoard: display real via BSP + esp_lvgl_port, com a receita de
// plataforma paga em bancada (RESOURCE-BUDGET §2, docs/PATRIMONIO-TECNICO.md).
//
// Receita fixada aqui:
//   - draw buffer parcial de 60 linhas em PSRAM (buff_spiram=true) — é a
//     origem de DMA que a hipótese 2.1 do glitch acusa (GLITCH-PROTOCOLO §2.1);
//   - double_buffer=false (RESOURCE-BUDGET §2.5);
//   - rotação 180° por PPA (sw_rotate=true);
//   - RGB565 (CONFIG_LV_COLOR_DEPTH_16).
#include "board/waveshare_board.hpp"

#include "bsp/display.h"
#include "bsp/esp-bsp.h"
#include "esp_log.h"
#include "esp_lv_adapter.h"
#include "esp_lv_adapter_display.h"
#include "lvgl.h"
#include "sdkconfig.h"

namespace nova {
namespace board {

namespace {
constexpr const char* kTag = "board.ws";

// Rotação de 180°: o painel é montado invertido. O adapter faz rotação E
// anti-tearing no MESMO pipeline (PPA + troca de framebuffer no momento seguro),
// que é o que o esp_lvgl_port não permitia (sw_rotate incompatível com
// full_refresh). Ver ADR-026.
constexpr esp_lv_adapter_rotation_t kRotation = ESP_LV_ADAPTER_ROTATE_180;

// TRIPLE_PARTIAL: é o DEFAULT do adapter para MIPI-DSI
// (ESP_LV_ADAPTER_TEAR_AVOID_MODE_DEFAULT_MIPI_DSI), ou seja, o caminho testado.
//
// DOUBLE_DIRECT foi tentado primeiro (melhor para área pequena) e NÃO funciona
// com rotação nesta versão 0.5.3: `required_frame_buffer_count()` devolve 2 para
// rotação 180 (display_manager.c:1423), o fetch grava frame_buffer_count=2, e
// então `use_panel_buffers()` exige >2 (linha 1017) e falha. O adapter cai em
// buffer parcial e o LVGL estoura "DIRECT mode requires screen sized buffer(s)"
// -> watchdog. Nenhum ajuste de Kconfig resolve: a contagem é interna.
// TRIPLE_PARTIAL pede 3 buffers de verdade e usa caminho de render parcial.
constexpr esp_lv_adapter_tear_avoid_mode_t kTearMode =
    ESP_LV_ADAPTER_TEAR_AVOID_MODE_TRIPLE_PARTIAL;

// Pilha da task do adapter: 16 KB é PONTO DE PARTIDA para medir a marca d'água
// (RESOURCE-BUDGET §2.1), não número confirmado.
constexpr uint32_t kUiTaskStackBytes = 16 * 1024;

// As macros de config do esp_lvgl_adapter usam inicializadores designados que
// não cobrem todos os campos, o que dispara -Wmissing-field-initializers ao
// compilar em C++. Os campos omitidos são zero-inicializados pela linguagem, ou
// seja, o comportamento está correto — é ruído de terceiro. Silenciado só aqui,
// para não mascarar warnings do NOSSO código.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"

bool init_adapter() {
    esp_lv_adapter_config_t cfg = ESP_LV_ADAPTER_DEFAULT_CONFIG();
    cfg.task_stack_size = kUiTaskStackBytes;
    cfg.task_priority = 4;  // net_worker (futuro) fica ABAIXO disto (§5.5)
    if (esp_lv_adapter_init(&cfg) != ESP_OK) {
        ESP_LOGE(kTag, "esp_lv_adapter_init falhou");
        return false;
    }
    return true;
}

// O painel DPI é criado pelo BSP ANTES daqui, com num_fbs vindo do Kconfig.
// Por isso não dá para "passar" o número: o que se faz é PERGUNTAR ao adapter
// quantos ele precisa e VALIDAR contra o Kconfig, falhando ruidosamente se
// divergir — em vez de fixar um valor no código.
bool check_frame_buffers() {
    uint8_t need = esp_lv_adapter_get_required_frame_buffer_count(kTearMode, kRotation);
    // CORREÇÃO de inconsistência do esp_lvgl_adapter 0.5.3: a função acima só
    // devolve 3 para rotação 90/270 (display_manager.c:1423), mas
    // display_manager_use_panel_buffers() exige >2 para QUALQUER rotação != 0
    // (display_manager.c:1017). Com 2 o setup falha em silêncio e o LVGL estoura
    // "DIRECT mode requires screen sized buffer(s)" -> watchdog. Comprovado.
    if (kRotation != ESP_LV_ADAPTER_ROTATE_0 && need < 3) {
        ESP_LOGW(kTag, "adapter pediu %u fb, mas rotacao != 0 exige 3; usando 3", need);
        need = 3;
    }
    const int have = CONFIG_BSP_LCD_DPI_BUFFER_NUMS;
    ESP_LOGI(kTag, "framebuffers: necessario %u, BSP criou %d", need, have);
    if (have < need) {
        ESP_LOGE(kTag, "CONFIG_BSP_LCD_DPI_BUFFER_NUMS=%d < %u exigido -> ajuste o sdkconfig",
                 have, need);
        return false;
    }
    return true;
}

lv_display_t* add_display(const bsp_lcd_handles_t& h) {
    esp_lv_adapter_display_config_t cfg = ESP_LV_ADAPTER_DISPLAY_MIPI_DEFAULT_CONFIG(
        h.panel, h.io, BSP_LCD_H_RES, BSP_LCD_V_RES, kRotation);
    cfg.tear_avoid_mode = kTearMode;
    ESP_LOGI(kTag, "adapter: rotacao=%d tear_mode=%d", static_cast<int>(kRotation),
             static_cast<int>(kTearMode));
    return esp_lv_adapter_register_display(&cfg);
}

#pragma GCC diagnostic pop
}  // namespace

bool WaveshareBoard::init_display() {
    // O adapter precisa existir antes de registrar o display, e a checagem de
    // framebuffers precisa acontecer antes de o BSP criar o painel.
    if (!check_frame_buffers() || !init_adapter()) {
        return false;  // NÃO abortar (ARCHITECTURE §8)
    }
    bsp_lcd_handles_t h = {};
    if (bsp_display_new_with_handles(nullptr, &h) != ESP_OK) {
        ESP_LOGE(kTag, "bsp_display_new_with_handles falhou");
        return false;
    }
    if (bsp_display_brightness_init() != ESP_OK) {
        ESP_LOGW(kTag, "brightness_init falhou; backlight seguira travado");
    }
    disp_ = add_display(h);
    if (disp_ == nullptr) {
        ESP_LOGE(kTag, "esp_lv_adapter_register_display falhou");
        return false;
    }
    // A rotação é declarada na config do display (kRotation) e executada pelo
    // adapter no seu pipeline — NÃO se chama lv_display_set_rotation aqui.
    if (esp_lv_adapter_start() != ESP_OK) {
        ESP_LOGE(kTag, "esp_lv_adapter_start falhou");
        return false;
    }
    return true;
}

// lock_ui e lock_shared_i2c compartilham o MESMO lock por baixo: touch e codec
// dividem o I2C e o polling de touch roda dentro do lock do display
// (RESOURCE-BUDGET §6). Esse invariante mora AQUI e em nenhum outro lugar.
// timeout 0 = espera indefinida no esp_lvgl_port; no adapter, -1. Traduzido aqui
// para a HAL manter a mesma semântica externa.
namespace {
int32_t to_adapter_timeout(uint32_t ms) {
    return ms == 0 ? -1 : static_cast<int32_t>(ms);
}
}  // namespace

bool WaveshareBoard::lock_ui(uint32_t timeout_ms) {
    return esp_lv_adapter_lock(to_adapter_timeout(timeout_ms)) == ESP_OK;
}
void WaveshareBoard::unlock_ui() { esp_lv_adapter_unlock(); }
bool WaveshareBoard::lock_shared_i2c(uint32_t timeout_ms) {
    return esp_lv_adapter_lock(to_adapter_timeout(timeout_ms)) == ESP_OK;
}
void WaveshareBoard::unlock_shared_i2c() { esp_lv_adapter_unlock(); }

void WaveshareBoard::set_brightness(int pct) { bsp_display_brightness_set(pct); }

uint64_t WaveshareBoard::rtc_unix_time_s() {
    // RTC com bateria entra na Onda A (ADR-015). Por ora, sem fonte plausível.
    return 0;
}

DrawBufferReport WaveshareBoard::describe_draw_buffers() {
    DrawBufferReport rep;
    if (disp_ == nullptr) {
        return rep;
    }
    // Lido via API pública do LVGL, sem header privado. Com o adapter, o buffer
    // ativo é um dos framebuffers do painel (tamanho de tela cheia).
    lv_draw_buf_t* b = lv_display_get_buf_active(disp_);
    if (b != nullptr && b->data != nullptr) {
        rep.buffers_[0].base_ = reinterpret_cast<uintptr_t>(b->data);
        rep.buffers_[0].size_ = b->data_size;
        rep.count_ = 1;
    }
    return rep;
}

}  // namespace board
}  // namespace nova
