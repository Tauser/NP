// Adaptador alvo-only para a porta USB fisica. Nao cria socket, AP, portal ou
// endpoint de LAN; so entrega um frame local para a caixa consumida pela app_loop.
#pragma once

#include "services/wifi_provisioning_mailbox.hpp"

namespace nova {
namespace services {

class UsbWifiProvisioner final {
public:
    explicit UsbWifiProvisioner(WifiProvisioningMailbox& mailbox) : mailbox_(mailbox) {}

    bool start();

private:
    static void task_entry(void* context);
    void run();

    WifiProvisioningMailbox& mailbox_;
    bool started_ = false;  // dono: app_loop, antes de criar a task
};

}  // namespace services
}  // namespace nova
