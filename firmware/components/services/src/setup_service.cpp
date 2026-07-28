#include "services/setup_service.hpp"

#include <cstring>

namespace nova {
namespace services {

SetupService::SetupService(core::StateStore& store, board::IBoard& board,
                           IWifiCredentialsStore& credentials,
                           WifiProvisioningMailbox& provisioning_mailbox)
    : store_(store), board_(board), credentials_store_(credentials),
      provisioning_mailbox_(provisioning_mailbox) {}

utils::Status SetupService::start() {
    const auto loaded = credentials_store_.load();
    if (!loaded.is_ok()) {
        // NVS corrompido/indisponivel nao pode impedir ClockService nem o
        // restante do app_loop de iniciar. A degradacao fica explicita no
        // estado, e nenhuma tentativa de associacao usa dado nao validado.
        if (loaded.status() != utils::Status::kNotFound) {
            publish(models::WifiSetupPhase::kFailed, false, 0);
            return utils::Status::kOk;
        }
        publish(models::WifiSetupPhase::kUnconfigured, false, 0);
        return utils::Status::kOk;
    }
    const utils::Status valid = validate_credentials(loaded.value());
    if (valid != utils::Status::kOk) {
        publish(models::WifiSetupPhase::kFailed, false, 0);
        return utils::Status::kOk;
    }
    credentials_ = loaded.value();
    has_credentials_ = true;
    credentials_saved_ = true;
    publish(models::WifiSetupPhase::kAssociating, true, 0);
    return utils::Status::kOk;
}

void SetupService::tick(uint64_t now_ms) {
    board::WifiCredentials provisioned;
    if (provisioning_mailbox_.take(provisioned)) {
        const utils::Status result = submit_wifi_credentials(provisioned);
        provisioned = {};
        if (result != utils::Status::kOk) {
            publish(models::WifiSetupPhase::kFailed, credentials_saved_, now_ms);
            return;
        }
    }
    if (!has_credentials_) return;
    if (!association_started_ && board_.network_transport_ready() && now_ms >= next_attempt_ms_) {
        begin_association(now_ms);
    }

    switch (board_.wifi_connection_state()) {
        case board::WifiConnectionState::kConnected:
            if (connected_since_ms_ == UINT64_MAX) connected_since_ms_ = now_ms;
            store_.set_network(models::NetworkState::kUp);
            if (!credentials_saved_ && now_ms - connected_since_ms_ >= kPersistAfterConnectedMs) {
                if (credentials_store_.save(credentials_) == utils::Status::kOk) credentials_saved_ = true;
            }
            publish(models::WifiSetupPhase::kConnected, credentials_saved_, now_ms);
            break;
        case board::WifiConnectionState::kFailed:
            store_.set_network(models::NetworkState::kDown);
            publish(models::WifiSetupPhase::kFailed, credentials_saved_, now_ms);
            connected_since_ms_ = UINT64_MAX;
            if (association_started_) {
                association_started_ = false;
                next_attempt_ms_ = now_ms + retry_delay_ms_;
                retry_delay_ms_ = retry_delay_ms_ < kMaxRetryMs / 2 ? retry_delay_ms_ * 2 : kMaxRetryMs;
            }
            break;
        case board::WifiConnectionState::kAssociating:
            store_.set_network(models::NetworkState::kConnecting);
            publish(models::WifiSetupPhase::kAssociating, credentials_saved_, now_ms);
            break;
        case board::WifiConnectionState::kIdle:
            break;
    }
}

utils::Status SetupService::submit_wifi_credentials(board::WifiCredentials credentials) {
    const utils::Status valid = validate_credentials(credentials);
    if (valid != utils::Status::kOk) return valid;
    const bool already_saved = credentials_saved_ && credentials_equal(credentials_, credentials);
    credentials_ = credentials;
    has_credentials_ = true;
    credentials_saved_ = already_saved;
    association_started_ = false;
    connected_since_ms_ = UINT64_MAX;
    next_attempt_ms_ = 0;
    retry_delay_ms_ = kInitialRetryMs;
    return utils::Status::kOk;
}

utils::Status SetupService::validate_credentials(const board::WifiCredentials& credentials) {
    const auto bounded_length = [](const char* text, size_t capacity) {
        size_t length = 0;
        while (length < capacity && text[length] != '\0') ++length;
        return length;
    };
    const size_t ssid_len = bounded_length(credentials.ssid_, sizeof(credentials.ssid_));
    const size_t pass_len = bounded_length(credentials.passphrase_, sizeof(credentials.passphrase_));
    if (ssid_len == 0 || ssid_len == sizeof(credentials.ssid_) ||
        pass_len == sizeof(credentials.passphrase_) || (pass_len != 0 && pass_len < 8)) {
        return utils::Status::kInvalidArg;
    }
    return utils::Status::kOk;
}

void SetupService::publish(models::WifiSetupPhase phase, bool saved, uint64_t now_ms) {
    if (has_published_ && published_phase_ == phase && published_saved_ == saved) return;
    store_.set_wifi_setup({now_ms, phase, saved});
    published_phase_ = phase;
    published_saved_ = saved;
    has_published_ = true;
}

void SetupService::begin_association(uint64_t now_ms) {
    association_started_ = board_.start_wifi_station(credentials_);
    if (!association_started_) {
        publish(models::WifiSetupPhase::kFailed, credentials_saved_, now_ms);
        next_attempt_ms_ = now_ms + retry_delay_ms_;
        retry_delay_ms_ = retry_delay_ms_ < kMaxRetryMs / 2 ? retry_delay_ms_ * 2 : kMaxRetryMs;
    }
}

bool SetupService::credentials_equal(const board::WifiCredentials& left,
                                     const board::WifiCredentials& right) {
    return std::memcmp(left.ssid_, right.ssid_, sizeof(left.ssid_)) == 0 &&
           std::memcmp(left.passphrase_, right.passphrase_, sizeof(left.passphrase_)) == 0;
}

}  // namespace services
}  // namespace nova
