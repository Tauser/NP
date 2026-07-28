// Testes da regra de setup sem IDF: credencial fica fora do estado, so grava
// depois de IP estavel e uma falha de associacao nao derruba o app_loop.
#include <cstdio>
#include <cstring>

#include "board/mock_board.hpp"
#include "core/state_store.hpp"
#include "services/setup_service.hpp"
#include "services/usb_wifi_provisioning_protocol.hpp"
#include "services/wifi_credentials_store.hpp"

namespace {
int g_fail = 0;

void check(bool condition, const char* what) {
    if (!condition) {
        std::printf("  FAIL: %s\n", what);
        ++g_fail;
    }
}

class FakeLock final : public nova::core::ILock {
public:
    void lock() override {}
    void unlock() override {}
};

nova::board::WifiCredentials valid_credentials() {
    nova::board::WifiCredentials credentials;
    std::strcpy(credentials.ssid_, "rede-teste");
    std::strcpy(credentials.passphrase_, "senha-segura");
    return credentials;
}

void test_delayed_save_and_retry() {
    using namespace nova;
    FakeLock lock;
    core::StateStore state(lock);
    board::MockBoard board;
    services::MockWifiCredentialsStore storage;
    services::WifiProvisioningMailbox mailbox(lock);
    services::SetupService service(state, board, storage, mailbox);

    check(service.start() == utils::Status::kOk, "inicio sem NVS segue degradado");
    check(state.wifi_setup().phase_ == models::WifiSetupPhase::kUnconfigured,
          "estado inicial e unconfigured");
    check(service.submit_wifi_credentials(valid_credentials()) == utils::Status::kOk,
          "aceita credencial valida");
    board.start_network_transport_async();
    service.tick(100);
    check(board.wifi_start_attempts_ == 1, "associa somente depois do enlace");
    check(state.network() == models::NetworkState::kConnecting, "associacao marca network connecting");

    board.set_wifi_connection_state(board::WifiConnectionState::kConnected);
    service.tick(1000);
    check(storage.saves() == 0, "nao persiste no instante do IP");
    service.tick(30999);
    check(storage.saves() == 0, "nao persiste antes de 30 s estaveis");
    service.tick(31000);
    check(storage.saves() == 1, "persiste uma vez apos 30 s estaveis");
    check(state.wifi_setup().has_saved_credentials_, "estado sinaliza credencial persistida");
    service.tick(60000);
    check(storage.saves() == 1, "nao regrava credencial identica");

    board.set_wifi_connection_state(board::WifiConnectionState::kFailed);
    service.tick(61000);
    check(state.wifi_setup().phase_ == models::WifiSetupPhase::kFailed,
          "falha fica explicita no estado");
    service.tick(62999);
    check(board.wifi_start_attempts_ == 1, "backoff evita retry em rajada");
    service.tick(63000);
    check(board.wifi_start_attempts_ == 2, "retry reabre associacao apos backoff");
}

void test_invalid_and_unavailable_storage() {
    using namespace nova;
    FakeLock lock;
    core::StateStore state(lock);
    board::MockBoard board;
    services::MockWifiCredentialsStore storage;
    services::WifiProvisioningMailbox mailbox(lock);
    services::SetupService service(state, board, storage, mailbox);
    board::WifiCredentials invalid;
    std::strcpy(invalid.ssid_, "rede");
    std::strcpy(invalid.passphrase_, "curta");
    check(service.submit_wifi_credentials(invalid) == utils::Status::kInvalidArg,
          "rejeita senha WPA curta");

    storage.set_failure(utils::Status::kInternal);
    check(service.start() == utils::Status::kOk, "NVS indisponivel nao para ServiceManager");
    check(state.wifi_setup().phase_ == models::WifiSetupPhase::kFailed,
          "NVS indisponivel aparece como setup falho");
}

void test_usb_protocol_and_mailbox() {
    using namespace nova;
    FakeLock lock;
    services::WifiProvisioningMailbox mailbox(lock);
    board::WifiCredentials credentials;
    constexpr char kFrame[] = "NPW1 cmVkZS10ZXN0ZQ== c2VuaGEtc2VndXJh";
    check(services::parse_usb_wifi_provisioning_frame(kFrame, sizeof(kFrame) - 1, credentials) ==
              utils::Status::kOk,
          "frame USB base64 valido faz parse");
    check(std::strcmp(credentials.ssid_, "rede-teste") == 0, "SSID decodificado");
    check(std::strcmp(credentials.passphrase_, "senha-segura") == 0, "senha decodificada");
    check(mailbox.submit(credentials), "caixa aceita primeira credencial");
    check(!mailbox.submit(credentials), "caixa nao sobrescreve segredo pendente");
    board::WifiCredentials received;
    check(mailbox.take(received), "app_loop consome credencial pendente");
    check(!mailbox.take(received), "caixa fica vazia apos consumo");
    check(services::parse_usb_wifi_provisioning_frame("NPW1 invalido", 13, credentials) ==
              utils::Status::kMalformed,
          "frame USB malformado e recusado");
}
}  // namespace

int main() {
    std::printf("setup service tests:\n");
    test_delayed_save_and_retry();
    test_invalid_and_unavailable_storage();
    test_usb_protocol_and_mailbox();
    if (g_fail == 0) {
        std::printf("  PASS (todos)\n");
        return 0;
    }
    std::printf("  %d checagens falharam\n", g_fail);
    return 1;
}
