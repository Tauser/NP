// ClockService deve ser puro fora da HAL: RTC inicializa offline; NTP apenas
// refinaria a hora quando a app_loop entregar uma amostra valida (ADR-015).
#include <cstdio>

#include "board/mock_board.hpp"
#include "core/lock.hpp"
#include "core/state_store.hpp"
#include "services/clock_service.hpp"

namespace {
int g_fail = 0;

void check(bool condition, const char* what) {
    if (!condition) {
        std::printf("  FAIL: %s\n", what);
        ++g_fail;
    }
}

void test_rtc_boot_and_minute_tick() {
    nova::core::NullLock lock;
    nova::core::StateStore store(lock);
    nova::board::MockBoard board;
    board.rtc_s_ = 1710000000ULL;  // 2024-03-09T16:00:00Z -> 13:00 BRT
    nova::services::ClockService service(store, board, {});

    check(service.start() == nova::utils::Status::kOk, "RTC plausivel inicia sem rede");
    auto clock = store.clock();
    check(clock.valid_ && !clock.stale_, "RTC plausivel produz hora utilizavel");
    check(clock.hour_ == 13 && clock.minute_ == 0, "offset Brasil aplicado no boot");
    check(clock.source_ == nova::models::ClockSource::kRtc, "fonte RTC declarada");

    service.tick(59999);
    check(store.clock().minute_ == 0, "nao repinta antes de virar o minuto");
    service.tick(60000);
    clock = store.clock();
    check(clock.hour_ == 13 && clock.minute_ == 1, "avanca relogio pelo monotonic clock");
    check(clock.last_update_ms_ == 60000, "registra instante monotonic da atualizacao");
}

void test_unavailable_and_ntp_handoff() {
    nova::core::NullLock lock;
    nova::core::StateStore store(lock);
    nova::board::MockBoard board;
    nova::services::ClockService service(store, board, {});

    check(service.start() == nova::utils::Status::kOk, "RTC ausente nao derruba o boot");
    auto clock = store.clock();
    check(!clock.valid_ && clock.stale_, "sem RTC mantem estado honesto indisponivel");
    check(service.accept_ntp_time({1}, 1000) == nova::utils::Status::kStale,
          "NTP implausivel e rejeitado");
    check(service.accept_ntp_time({1710003600ULL}, 120000) == nova::utils::Status::kOk,
          "amostra NTP plausivel e aceita");
    clock = store.clock();
    check(clock.hour_ == 14 && clock.minute_ == 0, "NTP atualiza hora local");
    check(clock.source_ == nova::models::ClockSource::kNtp && clock.valid_ && !clock.stale_,
          "NTP muda origem sem marcar dado como stale");
}

void test_invalid_timezone_is_loud() {
    nova::core::NullLock lock;
    nova::core::StateStore store(lock);
    nova::board::MockBoard board;
    nova::services::ClockService::Config bad_config;
    bad_config.utc_offset_minutes = 15 * 60;
    nova::services::ClockService service(store, board, bad_config);
    check(service.start() == nova::utils::Status::kInvalidArg, "timezone fora do limite falha explicitamente");
}
}  // namespace

int main() {
    std::printf("clock service tests:\n");
    test_rtc_boot_and_minute_tick();
    test_unavailable_and_ntp_handoff();
    test_invalid_timezone_is_loud();
    if (g_fail == 0) {
        std::printf("  PASS (todos)\n");
        return 0;
    }
    std::printf("  %d checagens falharam\n", g_fail);
    return 1;
}
