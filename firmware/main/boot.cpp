// Fiação da Onda A (docs/ARCHITECTURE.md §3/§8). Monta board, diagnóstico,
// worker de rede e instrumentação de render; ainda sem provider, NVS/cache ou
// tela de produto.
//
// Ordem (§8): display → primeiro frame → backlight. O backlight só liga DEPOIS
// do primeiro frame porque ligá-lo junto do display mostra tela branca no boot.
#include "boot.hpp"

#include "board/waveshare_board.hpp"
#include "cache/weather_cache.hpp"
#include "core/event_bus.hpp"
#include "core/lock.hpp"
#include "core/pump.hpp"
#include "core/service_manager.hpp"
#include "core/state_store.hpp"
#include "core/ui_dispatcher.hpp"
#include "diag/boot_diag.hpp"
#include "diag/flash_thrash.hpp"
#include "diag/render_probe.hpp"
#include "diag/render_workout.hpp"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "services/esp_http_client.hpp"
#include "services/clock_service.hpp"
#include "services/https_probe_service.hpp"
#include "services/network_worker.hpp"
#include "services/network_worker_task.hpp"
#include "services/setup_service.hpp"
#include "services/sntp_service.hpp"
#include "services/usb_wifi_provisioner.hpp"
#include "services/weather_service.hpp"
#include "services/wifi_credentials_store.hpp"
#include "services/wifi_provisioning_mailbox.hpp"
#include "providers/open_meteo_weather_provider.hpp"
#include "ui/screen_catalog.hpp"
#include "ui/shell.hpp"

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

// Período do tick da `app_loop`. Ela não renderiza e não faz busy-wait: dorme
// de verdade entre ciclos, porque consumo em repouso define a temperatura de
// regime num produto 24/7 (RESOURCE-BUDGET §8).
constexpr uint32_t kTickPeriodMs = 100;
constexpr services::ClockService::Config kClockConfig{};

// Estado e rede não devem tomar o lock da UI: isso acoplaria uma mutação curta
// do StateStore ao render e, quando o net_worker existir, deixaria TLS disputar
// o mesmo mutex da lvgl_task. Este adaptador fica no wiring porque `core/`
// conhece apenas ILock, nunca FreeRTOS.
class CoreMutex final : public core::ILock {
public:
    CoreMutex() : mutex_(xSemaphoreCreateMutexStatic(&storage_)) {}

    void lock() override { xSemaphoreTake(mutex_, portMAX_DELAY); }
    void unlock() override { xSemaphoreGive(mutex_); }

private:
    StaticSemaphore_t storage_ = {};
    SemaphoreHandle_t mutex_ = nullptr;
};

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

#ifndef NOVA_TORTURE
void start_network_transport(board::IBoard& board) {
    if (!board.start_network_transport_async()) {
        ESP_LOGE(kTag, "nao iniciou enlace ESP-Hosted; UI segue offline");
    }
}

void start_network_worker(services::NetworkWorkerTask& task) {
    if (!task.start()) {
        ESP_LOGE(kTag, "net_worker indisponivel; firmware permanece offline");
    }
}
#endif

void setup_shell(board::IBoard& board, core::UiDispatcher& dispatcher,
                 ui::ScreenRegistry& screens, ui::Shell& shell) {
    ui::register_screens(screens);
    if (!shell.attach(dispatcher)) {
        ESP_LOGE(kTag, "shell nao registrou todas as telas");
        return;
    }
    if (!board.lock_ui(0)) {
        ESP_LOGE(kTag, "lock_ui falhou; shell nao iniciou");
        return;
    }
    if (!shell.start()) {
        ESP_LOGE(kTag, "shell nao criou timer LVGL");
    }
    board.unlock_ui();
}
}  // namespace

void app_loop(board::IBoard& board);  // definida abaixo; não retorna

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

#ifndef NOVA_TORTURE
    // NVS antecede display por contrato de boot (§8). Se estiver indisponivel,
    // seguimos sem credenciais salvas; nao apagamos a particao automaticamente.
    if (services::initialize_wifi_credentials_storage() != utils::Status::kOk) {
        ESP_LOGE(kTag, "NVS indisponivel; Wi-Fi salvo fica desativado nesta sessao");
    }
#endif

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

#ifndef NOVA_TORTURE
    start_network_transport(board);
#endif

#ifdef NOVA_FLASH_THRASH
    // Só depois do render estar rodando: religa UM subsistema (flash) por cima
    // do render, para a bisseção do §3.2 (hipótese 2.5).
    diag::start_flash_thrash(kFlashThrashPeriodMs);
#endif

    app_loop(board);  // não retorna
}

// Ciclo da `app_loop` (ARCHITECTURE §5): a ÚNICA task que publica no EventBus e
// despacha invalidação de UI. Não renderiza — render é da task de UI (ADR-011).
//
// O passo do ciclo é `core::pump_once`, o MESMO código exercitado pelos testes
// de host. Reimplementar a sequência aqui faria os testes validarem uma cópia.
void app_loop(board::IBoard& board) {
    static CoreMutex lock;
    static core::EventBus bus;
    static core::StateStore store(lock);
    static core::UiDispatcher dispatcher;
    static core::ServiceManager services;
    static services::ClockService clock_service(store, board, kClockConfig);
#ifndef NOVA_TORTURE
    static core::RequestOrchestrator requests(lock, 400);
    static services::EspHttpClient http_client;
    static services::NetworkWorker worker(requests, http_client);
    static services::NetworkWorkerTask network_worker_task(worker);
    static services::SntpService sntp_service(board, clock_service);
    static services::HttpsProbeService https_probe_service(store, board, worker);
    static providers::OpenMeteoWeatherProvider weather_provider;
    static cache::LittlefsWeatherCache weather_cache;
    static services::WeatherService weather_service(store, board, worker, weather_provider, weather_cache);
    static services::NvsWifiCredentialsStore wifi_credentials;
    static services::WifiProvisioningMailbox wifi_provisioning_mailbox(lock);
    static services::SetupService setup_service(store, board, wifi_credentials,
                                                 wifi_provisioning_mailbox);
    static services::UsbWifiProvisioner usb_provisioner(wifi_provisioning_mailbox);
#endif
    static ui::ScreenRegistry screens;
    static ui::Shell shell(screens);

    if (!bus.subscribe(core::UiDispatcher::on_event, &dispatcher)) {
        ESP_LOGE(kTag, "dispatcher nao assinou o EventBus — UI nao atualizaria");
    }
    if (cache::initialize_weather_cache_storage() != utils::Status::kOk) {
        ESP_LOGW(kTag, "cache LittleFS indisponivel; clima segue sem persistencia offline");
    }
    setup_shell(board, dispatcher, screens, shell);
    if (services.register_service(clock_service) != utils::Status::kOk) {
        ESP_LOGE(kTag, "ClockService nao registrou; hora permanece indisponivel");
    }
#ifndef NOVA_TORTURE
    if (services.register_service(sntp_service) != utils::Status::kOk ||
        services.register_service(https_probe_service) != utils::Status::kOk ||
        services.register_service(weather_service) != utils::Status::kOk) {
        ESP_LOGE(kTag, "servicos NTP/HTTPS nao registraram; rede segue degradada");
    }
    if (services.register_service(setup_service) != utils::Status::kOk) {
        ESP_LOGE(kTag, "SetupService nao registrou; Wi-Fi permanece indisponivel");
    }
#endif
    if (services.start_all() != utils::Status::kOk) {
        ESP_LOGE(kTag, "ServiceManager nao iniciou; firmware permanece degradado");
    }
#ifndef NOVA_TORTURE
    if (!usb_provisioner.start()) {
        ESP_LOGE(kTag, "provisionamento USB indisponivel; Wi-Fi segue sem entrada local");
    }
    start_network_worker(network_worker_task);
#endif

    // Ainda NÃO há tela registrada: telas são da Onda C (ADR-023). O ciclo roda
    // vazio de propósito — o caminho está fechado e instrumentado, e a primeira
    // tela só precisa se registrar no dispatcher.
    ESP_LOGI(kTag, "app_loop iniciada (%u telas e 1 provider registrado)",
             static_cast<unsigned>(screens.count()));

    for (;;) {
        services.tick_all(static_cast<uint64_t>(esp_timer_get_time() / 1000));
        const size_t invalidated = core::pump_once(store, bus, dispatcher);
        if (invalidated > 0) {
            ESP_LOGD(kTag, "tick: %u telas invalidadas", static_cast<unsigned>(invalidated));
        }
        vTaskDelay(pdMS_TO_TICKS(kTickPeriodMs));
    }
}

}  // namespace app
}  // namespace nova
