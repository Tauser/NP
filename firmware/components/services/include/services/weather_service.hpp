// Agenda de clima: roda no app_loop, executa no unico net_worker e atualiza o
// StateStore. A app_loop e dona da leitura/escrita LittleFS; o net_worker so
// atualiza estado e pede persistencia pelo tick, evitando I/O de flash em TLS.
#pragma once

#include <atomic>
#include <cstdint>

#include "board/i_board.hpp"
#include "cache/weather_cache.hpp"
#include "core/service_manager.hpp"
#include "core/state_store.hpp"
#include "providers/i_weather_provider.hpp"
#include "services/network_worker.hpp"

namespace nova {
namespace services {

class WeatherService final : public core::IAppService, private IRequestHandler {
public:
    WeatherService(core::StateStore& store, board::IBoard& board, NetworkWorker& worker,
                   providers::IWeatherProvider& provider, cache::IWeatherCache& cache);

    utils::Status start() override;
    void tick(uint64_t now_ms) override;

private:
    utils::Status execute(core::RequestLease lease, utils::IHttpClient& client,
                          utils::BoundedHttpBody& body) override;
    void load_cache();
    void save_cache(uint64_t now_utc_s);
    void mark_stale();

    core::StateStore& store_;
    board::IBoard& board_;
    NetworkWorker& worker_;
    providers::IWeatherProvider& provider_;
    cache::IWeatherCache& cache_;
    core::RequestId request_id_ = core::kInvalidRequestId;
    std::atomic<uint64_t> now_ms_{0};  // app_loop escreve; net_worker le
    std::atomic<bool> cache_save_pending_{false};  // net_worker pede; app_loop grava
    uint64_t saved_cache_utc_s_ = 0;  // app_loop exclusivamente
    bool cache_loaded_ = false;
    bool enabled_ = false;
};

}  // namespace services
}  // namespace nova
