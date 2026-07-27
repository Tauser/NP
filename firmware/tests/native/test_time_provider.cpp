// Contrato de provider de tempo sem relógio, rede ou payload externo.
#include <cstdio>

#include "providers/mock_time_provider.hpp"

namespace {
int g_fail = 0;

void check(bool condition, const char* what) {
    if (!condition) {
        std::printf("  FAIL: %s\n", what);
        ++g_fail;
    }
}

void test_mock_contract() {
    nova::providers::MockTimeProvider provider({1710000000});
    const auto valid = provider.fetch_utc_time();
    check(valid.is_ok() && valid.value().unix_time_s == 1710000000, "devolve UTC configurado");
    provider.set_failure(nova::utils::Status::kNetworkDown);
    const auto failure = provider.fetch_utc_time();
    check(!failure.is_ok() && failure.status() == nova::utils::Status::kNetworkDown,
          "propaga falha sem inventar horario");
    provider.set_failure(nova::utils::Status::kOk);
    provider.set_time({1710000123});
    check(provider.fetch_utc_time().value().unix_time_s == 1710000123, "permite novo valor no mock");
}
}  // namespace

int main() {
    std::printf("time provider tests:\n");
    test_mock_contract();
    if (g_fail == 0) {
        std::printf("  PASS (todos)\n");
        return 0;
    }
    std::printf("  %d checagens falharam\n", g_fail);
    return 1;
}
