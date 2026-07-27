// WaveshareBoard: display real via BSP + esp_lvgl_adapter, com a receita de
// plataforma paga em bancada (RESOURCE-BUDGET §2, docs/PATRIMONIO-TECNICO.md).
//
// Receita vigente (ADR-026), validada em placa em 2026-07-26:
//   - backend de display: esp_lvgl_adapter (NÃO esp_lvgl_port, que não combina
//     sw_rotate com full_refresh e por isso obrigava a escolher entre rotação e
//     ausência de tearing);
//   - modo TRIPLE_PARTIAL: 3 framebuffers lidos pelo DSI + buffer de desenho
//     parcial de 50 linhas;
//   - rotação 180° pelo pipeline do adapter (o EK79007 ignora MADCTL, e
//     esp_lcd_panel_swap_xy não é suportado neste painel);
//   - RGB565 (CONFIG_LV_COLOR_DEPTH_16).
//
// O BSP continua criando o painel DSI; só o backend de LVGL mudou.
#include "board/waveshare_board.hpp"

#include "bsp/display.h"
#include "bsp/esp-bsp.h"
#include "esp_lcd_mipi_dsi.h"
#include "esp_log.h"
#include "esp_lv_adapter.h"
#include "esp_lv_adapter_display.h"
#include "esp_hosted.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
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

// RGB565 (CONFIG_LV_COLOR_DEPTH_16).
constexpr size_t kBytesPerPixel = 2;

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

// Guarda de compilação: no adapter 0.5.3 os modos DOUBLE_* são incompatíveis com
// rotação (ADR-026) e a falha aparece só em runtime, como task watchdog. Falhar
// aqui é barato; descobrir na bancada custou dois reflashes.
// Um upgrade do componente DEVE revalidar isto em placa antes de relaxar.
static_assert(kRotation == ESP_LV_ADAPTER_ROTATE_0 ||
                  (kTearMode != ESP_LV_ADAPTER_TEAR_AVOID_MODE_DOUBLE_DIRECT &&
                   kTearMode != ESP_LV_ADAPTER_TEAR_AVOID_MODE_DOUBLE_FULL &&
                   kTearMode != ESP_LV_ADAPTER_TEAR_AVOID_MODE_DOUBLE_PARTIAL),
              "adapter 0.5.3: modos DOUBLE_* nao funcionam com rotacao != 0 "
              "(ver ADR-026); use TRIPLE_PARTIAL");

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
    cfg.task_priority = 4;  // net_worker fica abaixo desta prioridade (§5.5)
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
    // NÃO chamar bsp_display_brightness_init() aqui: o próprio
    // bsp_display_new_with_handles() já o chama internamente
    // (esp32_p4_wifi6_touch_lcd_7b.c:416). A chamada duplicada reconfigurava o
    // canal LEDC e era a origem do aviso `ledc: GPIO 32 is not usable` que
    // aparecia em TODO boot — ruído que chegou a ser confundido com evidência
    // da hipótese 2.2 (backlight) do GLITCH-PROTOCOLO.
    panel_ = h.panel;
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

bool WaveshareBoard::start_network_transport_async() {
    if (transport_ready_.load()) {
        return true;
    }
    bool expected = false;
    if (!transport_starting_.compare_exchange_strong(expected, true)) {
        return true;
    }
    if (xTaskCreate(network_transport_task, "hosted_link", 4096, this, 3, nullptr) == pdPASS) {
        return true;
    }
    transport_starting_.store(false);
    ESP_LOGE(kTag, "nao criou task do enlace ESP-Hosted");
    return false;
}

bool WaveshareBoard::network_transport_ready() const { return transport_ready_.load(); }

void WaveshareBoard::network_transport_task(void* context) {
    auto* board = static_cast<WaveshareBoard*>(context);
    const int init_result = esp_hosted_init();
    const int connect_result = init_result == ESP_OK ? esp_hosted_connect_to_slave() : init_result;
    if (connect_result == ESP_OK) {
        board->transport_ready_.store(true);
        ESP_LOGI(kTag, "enlace P4<->C6 pronto; Wi-Fi ainda nao associado");
    } else {
        ESP_LOGE(kTag, "enlace ESP-Hosted falhou: %d", connect_result);
    }
    board->transport_starting_.store(false);
    vTaskDelete(nullptr);
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

    // (1) Os TRÊS framebuffers que o DSI lê continuamente. São eles que a
    // ADR-019 manda verificar: um framebuffer desalinhado é lido por DMA a
    // 73,7 MB/s, e o defeito apareceria como artefato intermitente, não como
    // erro. Consultar só o buffer ativo do LVGL cobriria 1 de 3.
    if (panel_ != nullptr) {
        void* fb[3] = {nullptr, nullptr, nullptr};
        const size_t fb_size =
            static_cast<size_t>(BSP_LCD_H_RES) * BSP_LCD_V_RES * kBytesPerPixel;
        if (esp_lcd_dpi_panel_get_frame_buffer(
                static_cast<esp_lcd_panel_handle_t>(panel_), 3, &fb[0], &fb[1], &fb[2]) == ESP_OK) {
            for (void* p : fb) {
                if (p != nullptr && rep.count_ < DrawBufferReport::kMaxBuffers) {
                    rep.buffers_[rep.count_].base_ = reinterpret_cast<uintptr_t>(p);
                    rep.buffers_[rep.count_].size_ = fb_size;
                    ++rep.count_;
                }
            }
        } else {
            ESP_LOGW(kTag, "nao foi possivel ler os framebuffers do painel");
        }
    }

    // (2) O buffer de desenho parcial do LVGL (origem do blit para o
    // framebuffer). Lido via API pública, sem header privado.
    if (disp_ != nullptr && rep.count_ < DrawBufferReport::kMaxBuffers) {
        lv_draw_buf_t* b = lv_display_get_buf_active(disp_);
        if (b != nullptr && b->data != nullptr) {
            rep.buffers_[rep.count_].base_ = reinterpret_cast<uintptr_t>(b->data);
            rep.buffers_[rep.count_].size_ = b->data_size;
            ++rep.count_;
        }
    }
    return rep;
}

}  // namespace board
}  // namespace nova
