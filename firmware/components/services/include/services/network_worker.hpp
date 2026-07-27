// NetworkWorker — único executor de requisições externas (ARCHITECTURE §7).
//
// PROPRIEDADE: `register_handler()` e `set_enabled()` pertencem ao app_loop
// durante o wiring, ANTES de `NetworkWorkerTask::start()`. Depois disso só a
// task `net_worker` chama `run_once()`. O buffer pertence à task e deve ser
// SRAM interna com exatamente pelo menos 48 KiB.
#pragma once

#include <cstddef>
#include <cstdint>

#include "core/request_orchestrator.hpp"
#include "utils/http_client.hpp"

namespace nova {
namespace services {

class IRequestHandler {
public:
    virtual ~IRequestHandler() = default;
    virtual utils::Status execute(core::RequestLease lease, utils::IHttpClient& client,
                                  utils::BoundedHttpBody& body) = 0;
};

class NetworkWorker {
public:
    NetworkWorker(core::RequestOrchestrator& orchestrator, utils::IHttpClient& client);

    // A task faz esta injeção uma vez, antes de entrar no loop. Explicitar o
    // buffer evita alocação invisível na API HTTP e permite o teste no host.
    bool configure_body_storage(uint8_t* storage, size_t capacity);
    utils::Result<core::RequestId> register_handler(core::RequestPolicy policy,
                                                    IRequestHandler& handler);
    bool set_enabled(core::RequestId id, bool enabled);

    // Executa no máximo uma lease. kBusy significa que não havia trabalho;
    // qualquer outro resultado já foi reportado ao orquestrador.
    utils::Status run_once(uint64_t now_ms);

private:
    core::RequestOrchestrator& orchestrator_;
    utils::IHttpClient& client_;
    IRequestHandler* handlers_[core::RequestOrchestrator::kMaxRequests] = {};
    uint8_t* storage_ = nullptr;
    size_t storage_capacity_ = 0;
};

}  // namespace services
}  // namespace nova
