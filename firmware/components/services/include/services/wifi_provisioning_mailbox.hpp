// Caixa de entrada de credencial USB. A tarefa de USB e a app_loop sao as
// unicas concorrentes; o mutex injetado protege o segredo e o header declara
// essa propriedade em vez de depender de convencao.
#pragma once

#include <cstdint>

#include "board/i_board.hpp"
#include "core/lock.hpp"

namespace nova {
namespace services {

class WifiProvisioningMailbox {
public:
    explicit WifiProvisioningMailbox(core::ILock& lock) : lock_(lock) {}

    // ESCRITOR: UsbWifiProvisioner. Retorna falso se houver uma credencial que
    // ainda nao foi consumida; nunca substitui segredo pendente em silencio.
    bool submit(board::WifiCredentials credentials);

    // LEITOR EXCLUSIVO: app_loop, via SetupService::tick(). Copia para o
    // chamador e apaga a copia transitória que ficava na caixa.
    bool take(board::WifiCredentials& out);

private:
    core::ILock& lock_;
    board::WifiCredentials pending_;
    bool has_pending_ = false;
};

}  // namespace services
}  // namespace nova
