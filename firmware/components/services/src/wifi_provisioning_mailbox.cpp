#include "services/wifi_provisioning_mailbox.hpp"

#include <cstring>

namespace nova {
namespace services {

bool WifiProvisioningMailbox::submit(board::WifiCredentials credentials) {
    core::LockGuard guard(lock_);
    if (has_pending_) return false;
    pending_ = credentials;
    has_pending_ = true;
    return true;
}

bool WifiProvisioningMailbox::take(board::WifiCredentials& out) {
    core::LockGuard guard(lock_);
    if (!has_pending_) return false;
    out = pending_;
    pending_ = {};
    has_pending_ = false;
    return true;
}

}  // namespace services
}  // namespace nova
