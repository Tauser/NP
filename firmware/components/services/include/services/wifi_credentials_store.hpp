#pragma once

#include "board/i_board.hpp"
#include "utils/result.hpp"

namespace nova {
namespace services {

// Inicializa somente a particao NVS padrao. Falha nao dispara erase automatico:
// preservar credenciais e deixar o painel em modo degradado e mais seguro que
// apagar segredo por uma falha de versao/espaco no boot.
utils::Status initialize_wifi_credentials_storage();

class IWifiCredentialsStore {
public:
    virtual ~IWifiCredentialsStore() = default;
    virtual utils::Result<board::WifiCredentials> load() = 0;
    virtual utils::Status save(const board::WifiCredentials& credentials) = 0;
};

class MockWifiCredentialsStore final : public IWifiCredentialsStore {
public:
    utils::Result<board::WifiCredentials> load() override;
    utils::Status save(const board::WifiCredentials& credentials) override;
    void set_failure(utils::Status status) { failure_ = status; }
    const board::WifiCredentials& saved() const { return saved_; }
    unsigned saves() const { return saves_; }

private:
    board::WifiCredentials saved_;
    utils::Status failure_ = utils::Status::kOk;
    bool has_saved_ = false;
    unsigned saves_ = 0;
};

class NvsWifiCredentialsStore final : public IWifiCredentialsStore {
public:
    utils::Result<board::WifiCredentials> load() override;
    utils::Status save(const board::WifiCredentials& credentials) override;
};

}  // namespace services
}  // namespace nova
