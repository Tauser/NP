#include "services/wifi_credentials_store.hpp"

namespace nova {
namespace services {

utils::Result<board::WifiCredentials> MockWifiCredentialsStore::load() {
    if (failure_ != utils::Status::kOk) return utils::Result<board::WifiCredentials>::fail(failure_);
    return has_saved_ ? utils::Result<board::WifiCredentials>::ok(saved_)
                      : utils::Result<board::WifiCredentials>::fail(utils::Status::kNotFound);
}

utils::Status MockWifiCredentialsStore::save(const board::WifiCredentials& credentials) {
    if (failure_ != utils::Status::kOk) return failure_;
    saved_ = credentials;
    has_saved_ = true;
    ++saves_;
    return utils::Status::kOk;
}

}  // namespace services
}  // namespace nova
