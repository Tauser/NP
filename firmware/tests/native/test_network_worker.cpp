// Testa a ponte execução->política sem IDF, socket ou alocação dinâmica.
#include <cstdio>

#include "core/lock.hpp"
#include "services/network_worker.hpp"

namespace {
int g_fail = 0;

void check(bool condition, const char* what) {
    if (!condition) {
        std::printf("  FAIL: %s\n", what);
        ++g_fail;
    }
}

class FakeClient final : public nova::utils::IHttpClient {
public:
    nova::utils::Result<nova::utils::HttpResponse> get(const nova::utils::HttpRequest& request,
                                                        nova::utils::BoundedHttpBody& body) override {
        ++calls_;
        if (request.url == nullptr) {
            return nova::utils::Result<nova::utils::HttpResponse>::fail(
                nova::utils::Status::kInvalidArg);
        }
        const uint8_t bytes[] = {'o', 'k'};
        return body.append(bytes, sizeof(bytes)) == nova::utils::Status::kOk
                   ? body.finish(200)
                   : nova::utils::Result<nova::utils::HttpResponse>::fail(body.status());
    }
    unsigned calls_ = 0;
};

class FakeHandler final : public nova::services::IRequestHandler {
public:
    nova::utils::Status execute(nova::core::RequestLease, nova::utils::IHttpClient& client,
                                nova::utils::BoundedHttpBody& body) override {
        const nova::utils::HttpRequest request{"https://example.test", 1000};
        const auto response = client.get(request, body);
        ++calls_;
        return response.is_ok() && response.value().body_size == 2 ? nova::utils::Status::kOk
                                                                     : response.status();
    }
    unsigned calls_ = 0;
};

nova::core::RequestPolicy policy() {
    nova::core::RequestPolicy value;
    value.min_interval_ms = 1000;
    value.initial_backoff_ms = 100;
    value.max_backoff_ms = 400;
    value.failures_to_open = 2;
    return value;
}

void test_serial_execution() {
    nova::core::NullLock lock;
    nova::core::RequestOrchestrator orchestrator(lock, 0);
    FakeClient client;
    FakeHandler handler;
    nova::services::NetworkWorker worker(orchestrator, client);
    uint8_t storage[nova::utils::kHttpBodyMaxBytes] = {};
    check(worker.configure_body_storage(storage, sizeof(storage)), "aceita buffer de 48 KiB");
    const auto id = worker.register_handler(policy(), handler);
    check(id.is_ok() && worker.set_enabled(id.value(), true), "registra e habilita handler");
    check(worker.run_once(0) == nova::utils::Status::kOk, "executa uma lease HTTP");
    check(client.calls_ == 1 && handler.calls_ == 1, "chama transporte uma vez");
    check(!orchestrator.has_active_request(), "conclusao libera lease global");
    check(worker.run_once(1) == nova::utils::Status::kBusy, "intervalo impede segundo HTTPS");
}
}  // namespace

int main() {
    std::printf("network worker tests:\n");
    test_serial_execution();
    if (g_fail == 0) {
        std::printf("  PASS (todos)\n");
        return 0;
    }
    std::printf("  %d checagens falharam\n", g_fail);
    return 1;
}
