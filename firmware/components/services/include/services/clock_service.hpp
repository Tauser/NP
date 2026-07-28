// ClockService — hora local offline-first (ADR-015).
//
// PROPRIEDADE: `start()`, `tick()` e `accept_ntp_time()` pertencem somente à
// app_loop. A futura integração SNTP deve enfileirar a entrega para a app_loop;
// chamar `accept_ntp_time()` da net_worker criaria corrida nos campos abaixo.
#pragma once

#include <cstdint>

#include "board/i_board.hpp"
#include "core/service_manager.hpp"
#include "core/state_store.hpp"
#include "models/time.hpp"

namespace nova {
namespace services {

class ClockService final : public core::IAppService {
public:
    struct Config {
        // ADR-017: o MVP e pt-BR/Brasil. A configuracao fica no wiring, e nao
        // escondida na regra de negocio, para a futura tela de setup altera-la.
        int16_t utc_offset_minutes = -180;
    };

    ClockService(core::StateStore& store, board::IBoard& board, Config config);

    utils::Status start() override;
    void tick(uint64_t now_ms) override;

    // Entrada da futura adaptacao SNTP. Nao inicia I/O, nao toca UI e aceita
    // apenas UTC plausivel; a fonte passa a ser NTP somente apos esta validacao.
    utils::Status accept_ntp_time(models::UtcTime utc_time, uint64_t now_ms);

    static bool is_plausible_utc(uint64_t unix_time_s);

private:
    static constexpr uint64_t kMinPlausibleUtcS = 1704067200ULL;  // 2024-01-01
    static constexpr uint64_t kMaxPlausibleUtcS = 4102444800ULL;  // 2100-01-01
    bool has_valid_offset() const;
    void publish_clock(uint64_t utc_time_s, uint64_t now_ms, models::ClockSource source);

    core::StateStore& store_;
    board::IBoard& board_;
    Config config_;
    uint64_t base_utc_time_s_ = 0;
    uint64_t base_monotonic_ms_ = 0;
    uint64_t last_published_minute_ = UINT64_MAX;
    models::ClockSource source_ = models::ClockSource::kUnavailable;
    bool has_time_ = false;
};

}  // namespace services
}  // namespace nova
