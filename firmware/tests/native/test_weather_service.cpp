// Fault injection do fluxo de clima: sem FreeRTOS, socket ou LittleFS real.
// Exercita o contrato que a UI consumirá: cache stale sobrevive a rede ruim e
// falha de provider nunca substitui um dado válido por lixo.
#include <cstdio>

#include "board/mock_board.hpp"
#include "cache/weather_cache.hpp"
#include "core/lock.hpp"
#include "core/request_orchestrator.hpp"
#include "core/state_store.hpp"
#include "providers/i_weather_provider.hpp"
#include "services/network_worker.hpp"
#include "services/weather_service.hpp"

namespace {
int g_fail = 0;
uint8_t g_body_storage[nova::utils::kHttpBodyMaxBytes] = {};

void check(bool condition, const char* what) {
    if (!condition) {
        std::printf("  FAIL: %s\n", what);
        ++g_fail;
    }
}

class FakeCache final : public nova::cache::IWeatherCache {
public:
    nova::utils::Result<nova::cache::WeatherCacheEntry> load() override {
        if (load_status_ != nova::utils::Status::kOk) {
            return nova::utils::Result<nova::cache::WeatherCacheEntry>::fail(load_status_);
        }
        return nova::utils::Result<nova::cache::WeatherCacheEntry>::ok(entry_);
    }

    nova::utils::Status save(const nova::cache::WeatherCacheEntry&) override {
        ++saves_;
        return nova::utils::Status::kOk;
    }

    nova::utils::Status load_status_ = nova::utils::Status::kNotFound;
    nova::cache::WeatherCacheEntry entry_{};
    unsigned saves_ = 0;
};

class FakeProvider final : public nova::providers::IWeatherProvider {
public:
    nova::utils::Result<nova::models::WeatherState> fetch_current(
        nova::utils::IHttpClient&, nova::utils::BoundedHttpBody&) override {
        ++calls_;
        if (status_ != nova::utils::Status::kOk) {
            return nova::utils::Result<nova::models::WeatherState>::fail(status_);
        }
        return nova::utils::Result<nova::models::WeatherState>::ok(weather_);
    }

    nova::utils::Status status_ = nova::utils::Status::kOk;
    nova::models::WeatherState weather_{0, 246, 231, 124, 3,
                                       nova::models::WeatherSource::kLive, true, true, false};
    unsigned calls_ = 0;
};

class FakeClient final : public nova::utils::IHttpClient {
public:
    nova::utils::Result<nova::utils::HttpResponse> get(const nova::utils::HttpRequest&,
                                                        nova::utils::BoundedHttpBody&) override {
        return nova::utils::Result<nova::utils::HttpResponse>::fail(nova::utils::Status::kInternal);
    }
};

nova::models::WeatherState live_weather(int16_t temperature) {
    return nova::models::WeatherState{1, temperature, 210, 80, 2,
                                      nova::models::WeatherSource::kLive, true, true, false};
}

void enable_live_fetch(nova::core::StateStore& store, nova::board::MockBoard& board,
                       nova::services::WeatherService& service) {
    board.start_network_transport_async();
    board.set_wifi_connection_state(nova::board::WifiConnectionState::kConnected);
    store.set_clock({0, 12, 0, nova::models::ClockSource::kNtp, true, false});
    service.tick(0);
}

void test_wifi_absent_uses_stale_cache() {
    using namespace nova;
    core::NullLock lock;
    core::StateStore store(lock);
    board::MockBoard board;
    core::RequestOrchestrator requests(lock, 0);
    FakeClient client;
    services::NetworkWorker worker(requests, client);
    FakeProvider provider;
    FakeCache cache;
    cache.load_status_ = utils::Status::kOk;
    cache.entry_ = {live_weather(198), 1780000000ULL};
    cache.entry_.weather_.source_ = models::WeatherSource::kCache;
    cache.entry_.weather_.stale_ = true;
    services::WeatherService service(store, board, worker, provider, cache);

    check(worker.configure_body_storage(g_body_storage, sizeof(g_body_storage)), "configura corpo HTTP");
    check(service.start() == utils::Status::kOk, "registra clima mesmo sem Wi-Fi");
    service.tick(100);
    const models::WeatherState weather = store.weather();
    check(weather.valid_ && weather.stale_ && weather.source_ == models::WeatherSource::kCache,
          "Wi-Fi ausente mantem cache marcado stale");
    check(provider.calls_ == 0, "Wi-Fi ausente nao abre request");
}

void test_dns_failure_opens_and_recovers_breaker() {
    using namespace nova;
    core::NullLock lock;
    core::StateStore store(lock);
    store.set_weather(live_weather(210));
    board::MockBoard board;
    core::RequestOrchestrator requests(lock, 0);
    FakeClient client;
    services::NetworkWorker worker(requests, client);
    FakeProvider provider;
    provider.status_ = utils::Status::kNetworkDown;
    FakeCache cache;
    services::WeatherService service(store, board, worker, provider, cache);

    check(worker.configure_body_storage(g_body_storage, sizeof(g_body_storage)), "configura corpo HTTP");
    check(service.start() == utils::Status::kOk, "registra clima");
    enable_live_fetch(store, board, service);

    check(worker.run_once(0) == utils::Status::kNetworkDown, "falha DNS simulada chega ao worker");
    const models::WeatherState stale = store.weather();
    check(stale.valid_ && stale.stale_ && stale.temperature_deci_c_ == 210,
          "falha preserva leitura anterior e a marca stale");
    service.tick(5000);
    check(worker.run_once(5000) == utils::Status::kNetworkDown, "primeiro retry respeita backoff");
    service.tick(15000);
    check(worker.run_once(15000) == utils::Status::kNetworkDown, "segundo retry falha");
    check(requests.circuit_state(0) == core::CircuitState::kOpen, "tres falhas DNS abrem breaker");

    provider.status_ = utils::Status::kOk;
    service.tick(35000);
    check(worker.run_once(35000) == utils::Status::kOk, "probe half-open recupera quando DNS volta");
    const models::WeatherState recovered = store.weather();
    check(!recovered.stale_ && recovered.source_ == models::WeatherSource::kLive &&
              recovered.temperature_deci_c_ == 246,
          "recuperacao substitui cache stale por leitura live");
    check(requests.circuit_state(0) == core::CircuitState::kClosed, "sucesso fecha breaker");
}

void test_http_and_payload_failure_preserve_state() {
    using namespace nova;
    core::NullLock lock;
    core::StateStore store(lock);
    store.set_weather(live_weather(205));
    board::MockBoard board;
    core::RequestOrchestrator requests(lock, 0);
    FakeClient client;
    services::NetworkWorker worker(requests, client);
    FakeProvider provider;
    FakeCache cache;
    services::WeatherService service(store, board, worker, provider, cache);

    check(worker.configure_body_storage(g_body_storage, sizeof(g_body_storage)), "configura corpo HTTP");
    check(service.start() == utils::Status::kOk, "registra clima");
    enable_live_fetch(store, board, service);
    provider.status_ = utils::Status::kHttpError;
    check(worker.run_once(0) == utils::Status::kHttpError, "API 500 simulado falha");
    check(requests.circuit_state(0) == core::CircuitState::kOpen, "HTTP nao-2xx abre breaker conservador");
    check(store.weather().temperature_deci_c_ == 205 && store.weather().stale_,
          "HTTP 500 nao corrompe leitura anterior");

    provider.status_ = utils::Status::kOk;
    service.tick(900000);
    check(worker.run_once(900000) == utils::Status::kOk, "probe apos cooldown recupera API");

    provider.status_ = utils::Status::kMalformed;
    service.tick(2700000);
    check(worker.run_once(2700000) == utils::Status::kMalformed,
          "payload truncado ou malformado falha no worker");
    check(requests.circuit_state(0) == core::CircuitState::kOpen,
          "payload invalido abre breaker sem retries curtos");
    check(store.weather().valid_ && store.weather().stale_ &&
              store.weather().temperature_deci_c_ == 246,
          "payload invalido preserva ultimo dado live como stale");
}

void test_corrupt_cache_is_discarded() {
    using namespace nova;
    core::NullLock lock;
    core::StateStore store(lock);
    board::MockBoard board;
    core::RequestOrchestrator requests(lock, 0);
    FakeClient client;
    services::NetworkWorker worker(requests, client);
    FakeProvider provider;
    FakeCache cache;
    cache.load_status_ = utils::Status::kMalformed;
    services::WeatherService service(store, board, worker, provider, cache);

    check(service.start() == utils::Status::kOk, "cache corrompido nao impede registro do service");
    const models::WeatherState weather = store.weather();
    check(!weather.valid_ && weather.source_ == models::WeatherSource::kUnavailable,
          "cache corrompido e descartado sem inventar dado");
}
}  // namespace

int main() {
    std::printf("weather service fault-injection tests:\n");
    test_wifi_absent_uses_stale_cache();
    test_dns_failure_opens_and_recovers_breaker();
    test_http_and_payload_failure_preserve_state();
    test_corrupt_cache_is_discarded();
    if (g_fail == 0) {
        std::printf("  PASS (todos)\n");
        return 0;
    }
    std::printf("  %d checagens falharam\n", g_fail);
    return 1;
}
