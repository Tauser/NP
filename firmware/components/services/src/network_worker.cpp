#include "services/network_worker.hpp"

namespace nova {
namespace services {

NetworkWorker::NetworkWorker(core::RequestOrchestrator& orchestrator, utils::IHttpClient& client)
    : orchestrator_(orchestrator), client_(client) {}

bool NetworkWorker::configure_body_storage(uint8_t* storage, size_t capacity) {
    if (storage == nullptr || capacity < utils::kHttpBodyMaxBytes) {
        return false;
    }
    storage_ = storage;
    storage_capacity_ = capacity;
    return true;
}

utils::Result<core::RequestId> NetworkWorker::register_handler(core::RequestPolicy policy,
                                                                 IRequestHandler& handler) {
    const auto id = orchestrator_.register_request(policy);
    if (!id.is_ok()) {
        return id;
    }
    handlers_[id.value()] = &handler;
    return id;
}

bool NetworkWorker::set_enabled(core::RequestId id, bool enabled) {
    return id < core::RequestOrchestrator::kMaxRequests && handlers_[id] != nullptr &&
           orchestrator_.set_enabled(id, enabled);
}

utils::Status NetworkWorker::run_once(uint64_t now_ms) {
    const auto lease = orchestrator_.take_next(now_ms);
    if (!lease.is_ok()) {
        return lease.status();
    }
    const core::RequestId id = lease.value().id;
    utils::Status result = utils::Status::kInternal;
    if (storage_ == nullptr || storage_capacity_ < utils::kHttpBodyMaxBytes) {
        result = utils::Status::kNoMemory;
    } else if (id < core::RequestOrchestrator::kMaxRequests && handlers_[id] != nullptr) {
        utils::BoundedHttpBody body(storage_, storage_capacity_);
        result = body.status() == utils::Status::kOk
                     ? handlers_[id]->execute(lease.value(), client_, body)
                     : body.status();
    }
    return orchestrator_.complete(lease.value(), result, now_ms) ? result : utils::Status::kInternal;
}

}  // namespace services
}  // namespace nova
