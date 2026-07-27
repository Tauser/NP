// Testes puros da política de rede: sem HTTP, FreeRTOS ou placa.
#include <cstdio>

#include "core/request_orchestrator.hpp"

namespace {
int g_fail = 0;

void check(bool condition, const char* what) {
    if (!condition) {
        std::printf("  FAIL: %s\n", what);
        ++g_fail;
    }
}

class CountingLock : public nova::core::ILock {
public:
    void lock() override { ++locks_; }
    void unlock() override { ++unlocks_; }

    int locks_ = 0;
    int unlocks_ = 0;
};

nova::core::RequestPolicy policy(uint8_t priority = 0, uint8_t failures_to_open = 3,
                                  uint8_t jitter_percent = 0) {
    nova::core::RequestPolicy p;
    p.min_interval_ms = 1000;
    p.initial_backoff_ms = 100;
    p.max_backoff_ms = 400;
    p.failures_to_open = failures_to_open;
    p.priority = priority;
    p.jitter_percent = jitter_percent;
    return p;
}

uint32_t upper_jitter(uint32_t upper_inclusive, void*) { return upper_inclusive; }

void test_priority_and_lease() {
    using namespace nova;
    CountingLock lock;
    core::RequestOrchestrator requests(lock, 1000);
    core::RequestPolicy invalid = policy();
    invalid.max_backoff_ms = 99;
    check(!requests.register_request(invalid).is_ok(), "rejeita policy invalida");

    const auto low = requests.register_request(policy(2));
    const auto high = requests.register_request(policy(0));
    requests.set_enabled(low.value(), true);
    requests.set_enabled(high.value(), true);
    const auto first = requests.take_next(0);
    check(first.is_ok() && first.value().id == high.value(), "prioridade menor sai primeiro");
    check(!requests.take_next(0).is_ok(), "segunda lease simultanea recebe busy");
    check(!requests.complete(core::RequestLease{high.value(), 99}, utils::Status::kOk, 10),
          "lease errada e recusada");
    check(requests.complete(first.value(), utils::Status::kOk, 10), "conclusao valida aceita");
    check(!requests.take_next(999).is_ok(), "gap global bloqueia novo HTTPS");
    const auto second = requests.take_next(1000);
    check(second.is_ok() && second.value().id == low.value(), "gap libera proximo dominio");
    check(requests.complete(second.value(), utils::Status::kOk, 1000), "segunda conclusao aceita");
    check(lock.locks_ == lock.unlocks_, "locks balanceados");
}

void test_breaker() {
    using namespace nova;
    CountingLock lock;
    core::RequestOrchestrator breaker(lock, 0);
    const auto id = breaker.register_request(policy(0, 2));
    breaker.set_enabled(id.value(), true);
    const auto fail_one = breaker.take_next(0);
    breaker.complete(fail_one.value(), utils::Status::kTimeout, 0);
    check(breaker.circuit_state(id.value()) == core::CircuitState::kClosed, "primeira falha fecha");
    check(!breaker.take_next(99).is_ok(), "backoff inicial bloqueia retry cedo");
    const auto fail_two = breaker.take_next(100);
    breaker.complete(fail_two.value(), utils::Status::kNetworkDown, 100);
    check(breaker.circuit_state(id.value()) == core::CircuitState::kOpen, "limiar abre circuito");
    breaker.set_enabled(id.value(), false);
    breaker.set_enabled(id.value(), true);
    check(!breaker.take_next(299).is_ok(), "enable nao fura backoff aberto");
    const auto probe = breaker.take_next(300);
    check(probe.is_ok() && breaker.circuit_state(id.value()) == core::CircuitState::kHalfOpen,
          "prazo abre probe half-open");
    breaker.complete(probe.value(), utils::Status::kOk, 300);
    check(breaker.circuit_state(id.value()) == core::CircuitState::kClosed, "probe fecha circuito");
    check(!breaker.take_next(1299).is_ok(), "sucesso respeita intervalo minimo");

    core::RequestOrchestrator permanent(lock, 0);
    const auto permanent_id = permanent.register_request(policy(0, 3));
    permanent.set_enabled(permanent_id.value(), true);
    const auto malformed = permanent.take_next(0);
    permanent.complete(malformed.value(), utils::Status::kMalformed, 0);
    check(permanent.circuit_state(permanent_id.value()) == core::CircuitState::kOpen,
          "falha permanente abre breaker sem retries curtos");
    check(!permanent.take_next(399).is_ok(), "falha permanente respeita cooldown maximo");
}

void test_jitter() {
    using namespace nova;
    CountingLock lock;
    core::RequestOrchestrator requests(lock, 0, upper_jitter, nullptr);
    const auto id = requests.register_request(policy(0, 3, 50));
    requests.set_enabled(id.value(), true);
    const auto lease = requests.take_next(0);
    requests.complete(lease.value(), utils::Status::kNetworkDown, 0);
    check(!requests.take_next(149).is_ok(), "jitter positivo adia retry");
    check(requests.take_next(150).is_ok(), "jitter e determinista no host");
    check(lock.locks_ == lock.unlocks_, "jitter mantem locks balanceados");
}
}  // namespace

int main() {
    std::printf("request orchestrator tests:\n");
    test_priority_and_lease();
    test_breaker();
    test_jitter();
    if (g_fail == 0) {
        std::printf("  PASS (todos)\n");
        return 0;
    }
    std::printf("  %d checagens falharam\n", g_fail);
    return 1;
}
