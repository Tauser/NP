#pragma once

#include "board/i_board.hpp"
#include "core/service_manager.hpp"
#include "core/state_store.hpp"
#include "services/wifi_credentials_store.hpp"
#include "services/wifi_provisioning_mailbox.hpp"

namespace nova {
namespace services {

class SetupService final : public core::IAppService {
public:
    SetupService(core::StateStore& store, board::IBoard& board, IWifiCredentialsStore& credentials,
                 WifiProvisioningMailbox& provisioning_mailbox);

    utils::Status start() override;
    void tick(uint64_t now_ms) override;

    // PROPRIEDADE: somente app_loop. A futura UI envia a intenção para a
    // app_loop; credencial nunca percorre EventBus nem entra no AppState.
    utils::Status submit_wifi_credentials(board::WifiCredentials credentials);

    static utils::Status validate_credentials(const board::WifiCredentials& credentials);

private:
    static constexpr uint64_t kPersistAfterConnectedMs = 30000;
    static constexpr uint64_t kInitialRetryMs = 2000;
    static constexpr uint64_t kMaxRetryMs = 30000;
    void publish(models::WifiSetupPhase phase, bool saved, uint64_t now_ms);
    void begin_association(uint64_t now_ms);
    static bool credentials_equal(const board::WifiCredentials& left,
                                  const board::WifiCredentials& right);

    core::StateStore& store_;
    board::IBoard& board_;
    IWifiCredentialsStore& credentials_store_;
    WifiProvisioningMailbox& provisioning_mailbox_;
    board::WifiCredentials credentials_;
    uint64_t connected_since_ms_ = UINT64_MAX;
    bool has_credentials_ = false;
    bool credentials_saved_ = false;
    bool association_started_ = false;
    bool has_published_ = false;
    models::WifiSetupPhase published_phase_ = models::WifiSetupPhase::kUnconfigured;
    bool published_saved_ = false;
    uint64_t next_attempt_ms_ = 0;
    uint64_t retry_delay_ms_ = kInitialRetryMs;
};

}  // namespace services
}  // namespace nova
