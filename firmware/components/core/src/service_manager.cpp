#include "core/service_manager.hpp"

namespace nova {
namespace core {

utils::Status ServiceManager::register_service(IAppService& service) {
    if (sealed_) {
        return utils::Status::kBusy;
    }
    for (size_t i = 0; i < service_count_; ++i) {
        if (services_[i] == &service) {
            return utils::Status::kInvalidArg;
        }
    }
    if (service_count_ == kMaxServices) {
        return utils::Status::kNoMemory;
    }
    services_[service_count_++] = &service;
    return utils::Status::kOk;
}

utils::Status ServiceManager::start_all() {
    sealed_ = true;
    while (started_count_ < service_count_) {
        const utils::Status status = services_[started_count_]->start();
        if (status != utils::Status::kOk) {
            return status;
        }
        ++started_count_;
    }
    return utils::Status::kOk;
}

void ServiceManager::tick_all(uint64_t now_ms) {
    for (size_t i = 0; i < started_count_; ++i) {
        services_[i]->tick(now_ms);
    }
}

}  // namespace core
}  // namespace nova
