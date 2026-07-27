// Ciclo de vida de services sem FreeRTOS, alocação dinâmica ou hardware.
#include <cstdio>

#include "core/service_manager.hpp"

namespace {
int g_fail = 0;

void check(bool condition, const char* what) {
    if (!condition) {
        std::printf("  FAIL: %s\n", what);
        ++g_fail;
    }
}

class FakeService final : public nova::core::IAppService {
public:
    explicit FakeService(nova::utils::Status start_status = nova::utils::Status::kOk)
        : start_status_(start_status) {}

    nova::utils::Status start() override {
        ++starts_;
        return start_status_;
    }
    void tick(uint64_t now_ms) override {
        ++ticks_;
        last_tick_ms_ = now_ms;
    }

    nova::utils::Status start_status_;
    unsigned starts_ = 0;
    unsigned ticks_ = 0;
    uint64_t last_tick_ms_ = 0;
};

void test_lifecycle_and_seal() {
    nova::core::ServiceManager manager;
    FakeService first;
    FakeService second;
    check(manager.register_service(first) == nova::utils::Status::kOk, "registra primeiro");
    check(manager.register_service(first) == nova::utils::Status::kInvalidArg, "recusa registro duplicado");
    check(manager.register_service(second) == nova::utils::Status::kOk, "registra segundo");
    manager.tick_all(10);
    check(first.ticks_ == 0 && second.ticks_ == 0, "nao roda tick antes do start");
    check(manager.start_all() == nova::utils::Status::kOk, "inicia em ordem");
    check(first.starts_ == 1 && second.starts_ == 1 && manager.is_started(), "inicia cada um uma vez");
    manager.tick_all(123);
    check(first.last_tick_ms_ == 123 && second.last_tick_ms_ == 123, "propaga tick para iniciados");
    check(manager.register_service(first) == nova::utils::Status::kBusy, "sela registro apos start");
}

void test_failure_resumes_at_failed_service() {
    nova::core::ServiceManager manager;
    FakeService first;
    FakeService failing(nova::utils::Status::kNetworkDown);
    manager.register_service(first);
    manager.register_service(failing);
    check(manager.start_all() == nova::utils::Status::kNetworkDown, "propaga falha de start");
    check(first.starts_ == 1 && failing.starts_ == 1, "nao reinicia anterior na falha");
    manager.tick_all(7);
    check(first.ticks_ == 1 && failing.ticks_ == 0, "tick so alcanca inicializados");
    failing.start_status_ = nova::utils::Status::kOk;
    check(manager.start_all() == nova::utils::Status::kOk, "retry retoma no serviço falho");
    check(first.starts_ == 1 && failing.starts_ == 2, "retry preserva ordem e idempotencia");
}
}  // namespace

int main() {
    std::printf("service manager tests:\n");
    test_lifecycle_and_seal();
    test_failure_resumes_at_failed_service();
    if (g_fail == 0) {
        std::printf("  PASS (todos)\n");
        return 0;
    }
    std::printf("  %d checagens falharam\n", g_fail);
    return 1;
}
