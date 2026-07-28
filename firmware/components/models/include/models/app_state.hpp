// AppState — os fatos que a UI desenha (docs/ARCHITECTURE.md §6).
//
// PROPRIEDADE, declarada aqui porque o header é o lugar contratual (ADR-008):
//
//   ESCRITOR ÚNICO: `core::StateStore`, e somente pelos seus setters. Nenhum
//   outro componente muta estes campos — nem services, nem UI, nem a HAL.
//   LEITORES: recebem CÓPIA por valor via acessores granulares do StateStore.
//   Não existe getter que devolva esta struct inteira (ADR-011) e nenhum leitor
//   recebe referência ou ponteiro para ela.
//   SERIALIZAÇÃO: pelo ILock injetado no StateStore. Comentário afirmando
//   segurança sem mecanismo é proibido — já houve data race real assim.
//
// PURO. Cresce por domínio conforme as ondas avançam; hoje só tem o que a
// Onda A justifica (ADR-023: nada de campo para dado que não existe).
//
// CUSTO DE PILHA: campo novo aqui custa PILHA da task de render, não só RAM.
// A rotina de render lê ACESSORES GRANULARES e nunca copia esta struct inteira
// (ADR-011) — copiar já causou stack protection fault em campo
// (PATRIMONIO-TECNICO §5.1). Antes de crescer, refaça a conta do
// RESOURCE-BUDGET §2.1.
#pragma once

#include <cstdint>

namespace nova {
namespace models {

enum class NetworkState : uint8_t {
    kDown = 0,
    kConnecting,
    kUp,
};

enum class WifiSetupPhase : uint8_t {
    kUnconfigured = 0,
    kAssociating,
    kConnected,
    kFailed,
};

struct WifiSetupState {
    uint64_t last_change_ms_ = 0;
    WifiSetupPhase phase_ = WifiSetupPhase::kUnconfigured;
    bool has_saved_credentials_ = false;
};

enum class ClockSource : uint8_t {
    kUnavailable = 0,
    kRtc,
    kNtp,
};

struct ClockState {
    // Hora local já resolvida. `valid_` falso = sem hora plausível; a UI mostra
    // estado não-sincronizado e NUNCA inventa data (ADR-015).
    uint64_t last_update_ms_ = 0;
    uint8_t hour_ = 0;
    uint8_t minute_ = 0;
    ClockSource source_ = ClockSource::kUnavailable;
    bool valid_ = false;
    // RTC e NTP são fontes utilizáveis enquanto existirem; uma futura camada de
    // cache deve marcar este campo antes de publicar hora persistida.
    bool stale_ = true;
};

// Condição atual compacta para a primeira tela de clima. Temperaturas e vento
// são decimais para não introduzir ponto flutuante no estado/UI.
enum class WeatherSource : uint8_t {
    kUnavailable = 0,
    kLive,
    kCache,
    kMock,
};

struct WeatherState {
    uint64_t last_update_ms_ = 0;
    int16_t temperature_deci_c_ = 0;
    int16_t apparent_temperature_deci_c_ = 0;
    uint16_t wind_speed_deci_kmh_ = 0;
    uint8_t weather_code_ = 0;
    WeatherSource source_ = WeatherSource::kUnavailable;
    bool is_day_ = false;
    bool valid_ = false;
    bool stale_ = true;
};
static_assert(sizeof(WeatherState) == 24, "WeatherState deve permanecer compacto");

struct AppState {
    ClockState clock_;
    NetworkState network_ = NetworkState::kDown;
    WifiSetupState wifi_setup_;
    WeatherState weather_;
};

}  // namespace models
}  // namespace nova
