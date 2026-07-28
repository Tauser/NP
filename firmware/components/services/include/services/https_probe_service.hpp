// Sonda HTTPS única de bancada. Reusa o único net_worker e só inicia depois de
// NTP, para validar certificado sem concorrência de handshake TLS.
#pragma once

#include <atomic>

#include "board/i_board.hpp"
#include "core/service_manager.hpp"
#include "core/state_store.hpp"
#include "services/network_worker.hpp"

namespace nova {
namespace services {

class HttpsProbeService final : public core::IAppService, private IRequestHandler {
public:
    HttpsProbeService(core::StateStore& store, board::IBoard& board, NetworkWorker& worker);

    utils::Status start() override;
    void tick(uint64_t now_ms) override;

private:
    utils::Status execute(core::RequestLease lease, utils::IHttpClient& client,
                          utils::BoundedHttpBody& body) override;

    core::StateStore& store_;
    board::IBoard& board_;
    NetworkWorker& worker_;
    core::RequestId request_id_ = core::kInvalidRequestId;
    std::atomic<uint8_t> completed_{0};  // net_worker escreve; app_loop le
    bool enabled_ = false;
    bool finished_ = false;
};

}  // namespace services
}  // namespace nova
