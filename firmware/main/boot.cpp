// Fiação da Onda 0 (docs/ARCHITECTURE.md §3/§8). Monta board + diagnóstico +
// instrumentação de render. SEM rede, NVS, cache ou tela de produto (ADR-023).
//
// Ordem (§8): display → primeiro frame → backlight. O backlight só liga DEPOIS
// do primeiro frame porque ligá-lo junto do display mostra tela branca no boot.
#include "boot.hpp"

#include "board/waveshare_board.hpp"
#include "diag/boot_diag.hpp"
#include "diag/flash_thrash.hpp"
#include "diag/render_probe.hpp"
#include "diag/render_workout.hpp"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace nova {
namespace app {

namespace {
constexpr const char* kTag = "NovaPanel";

#if defined(NOVA_CLARITY)
// Desambiguação: churn mais lento (~2,5 Hz) para o carimbo ser legível a olho
// e em foto normal, mantendo flush suficiente.
constexpr bool kAggressive = true;
constexpr uint32_t kWorkoutPeriodMs = 400;
constexpr uint32_t kDumpPeriodMs = 2000;
#elif defined(NOVA_TORTURE)
// §3.2: só render, churn a ~10 Hz para maximizar flush.
constexpr bool kAggressive = true;
constexpr uint32_t kWorkoutPeriodMs = 100;
constexpr uint32_t kDumpPeriodMs = 2000;
#else
constexpr bool kAggressive = false;
constexpr uint32_t kWorkoutPeriodMs = 1000;
constexpr uint32_t kDumpPeriodMs = 5000;
#endif

constexpr int kBootBrightnessPct = 80;
constexpr int kFirstFrameTimeoutMs = 3000;

#ifdef NOVA_FLASH_THRASH
// Erase a cada 500 ms: frequente o bastante para casar com o render, espaçado
// o bastante para o olho/vídeo ligar UMA piscada a UM erase (§3.1).
constexpr uint32_t kFlashThrashPeriodMs = 500;
#endif

// Cria sonda e workout DENTRO do lock do UI: são chamadas LVGL e só a lvgl_task
// pode tocar objetos LVGL (ADR-011); o lock é a via sancionada de fora dela.
void setup_render(board::IBoard& board, diag::RenderMetrics& metrics) {
    if (!board.lock_ui(0)) {
        ESP_LOGE(kTag, "lock_ui falhou; sonda/workout nao instalados");
        return;
    }
    diag::attach_render_probe(metrics, kDumpPeriodMs);
    diag::start_render_workout(kAggressive, kWorkoutPeriodMs);
    board.unlock_ui();
}

void backlight_after_first_frame(board::IBoard& board) {
    int waited = 0;
    while (!diag::first_frame_rendered() && waited < kFirstFrameTimeoutMs) {
        vTaskDelay(pdMS_TO_TICKS(20));
        waited += 20;
    }
    if (diag::first_frame_rendered()) {
        ESP_LOGI(kTag, "primeiro frame em ~%d ms; ligando backlight", waited);
    } else {
        ESP_LOGW(kTag, "sem primeiro frame em %d ms; ligando backlight assim mesmo",
                 kFirstFrameTimeoutMs);
    }
    board.set_brightness(kBootBrightnessPct);
}
}  // namespace

void run() {
    ESP_LOGI(kTag, "NovaPanel — baseline 2026-07 — Onda 0 (atribuir glitch)");
#ifdef NOVA_TORTURE
    ESP_LOGW(kTag, "MODO TORTURE: so render, sem rede/NVS/cache (GLITCH §3.2)");
#endif
#ifdef NOVA_CLARITY
    ESP_LOGW(kTag, "MODO CLAREZA: carimbo grande/lento — topo+canto sao o conteudo");
#endif

    static board::WaveshareBoard board;
    static diag::RenderMetrics metrics;

    if (!board.init_display()) {
        // Falha de display NÃO chama abort() (§8). Retry/reboot com backoff é da
        // Onda A; aqui apenas registramos e paramos a fiação.
        ESP_LOGE(kTag, "init_display falhou — sem render nesta sessao");
        return;
    }

    // Diagnóstico de boot: endereços dos draw buffers, %64, DMA-safe e veredito
    // da hipótese 2.1 (GLITCH §2.1). Não interrompe o boot mesmo se DMA-unsafe:
    // um build de diagnóstico precisa continuar rodando para ser observado — o
    // erro aparece ruidoso no log (RESOURCE-BUDGET §2.6 / ADR-019).
    if (!diag::run_boot_diagnostic(board)) {
        ESP_LOGE(kTag, "diagnostico de boot acusou draw buffer DMA-unsafe (ver acima)");
    }

    setup_render(board, metrics);
    backlight_after_first_frame(board);

#ifdef NOVA_FLASH_THRASH
    // Só depois do render estar rodando: religa UM subsistema (flash) por cima
    // do render, para a bisseção do §3.2 (hipótese 2.5).
    diag::start_flash_thrash(kFlashThrashPeriodMs);
#endif
}

}  // namespace app
}  // namespace nova
