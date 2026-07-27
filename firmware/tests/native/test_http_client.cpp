// Testes do contrato de corpo HTTP: sem rede e sem alocação dinâmica.
#include <cstdio>
#include <cstring>

#include "utils/http_client.hpp"

namespace {
int g_fail = 0;
uint8_t g_storage[nova::utils::kHttpBodyMaxBytes] = {};
uint8_t g_full_response[nova::utils::kHttpBodyMaxBytes] = {};
uint8_t g_short_storage[nova::utils::kHttpBodyMaxBytes - 1] = {};

void check(bool condition, const char* what) {
    if (!condition) {
        std::printf("  FAIL: %s\n", what);
        ++g_fail;
    }
}

void test_valid_body() {
    using namespace nova::utils;
    BoundedHttpBody body(g_storage, sizeof(g_storage));
    const uint8_t part_a[] = {'o', 'k', ' '};
    const uint8_t part_b[] = {'v', '1'};
    check(body.append(part_a, sizeof(part_a)) == Status::kOk, "aceita primeiro chunk");
    check(body.append(part_b, sizeof(part_b)) == Status::kOk, "aceita segundo chunk");
    const Result<HttpResponse> response = body.finish(200);
    check(response.is_ok() && response.value().status_code == 200, "finaliza resposta valida");
    check(response.value().body_size == 5, "preserva tamanho acumulado");
    check(std::memcmp(response.value().body, "ok v1", 5) == 0, "preserva bytes do corpo");
}

void test_hard_limit() {
    using namespace nova::utils;
    BoundedHttpBody body(g_storage, sizeof(g_storage));
    const uint8_t one_byte[] = {1};
    check(body.append(g_full_response, sizeof(g_full_response)) == Status::kOk,
          "aceita exatamente 48 KiB");
    check(body.append(one_byte, sizeof(one_byte)) == Status::kTooLarge,
          "rejeita o byte acima do teto");
    check(!body.finish(200).is_ok() && body.status() == Status::kTooLarge,
          "corpo grande nao pode virar resposta parcial");
    check(body.reset() == Status::kOk && body.append(one_byte, sizeof(one_byte)) == Status::kOk,
          "reset recupera buffer depois da falha");
}

void test_buffer_contract() {
    using namespace nova::utils;
    BoundedHttpBody too_small(g_short_storage, sizeof(g_short_storage));
    check(too_small.status() == Status::kNoMemory, "recusa buffer menor que 48 KiB");
    check(!too_small.finish(200).is_ok(), "buffer invalido nao produz resposta");
    BoundedHttpBody valid(g_storage, sizeof(g_storage));
    check(valid.append(nullptr, 1) == Status::kInvalidArg, "chunk nulo com tamanho e invalido");
}
}  // namespace

int main() {
    std::printf("http client tests:\n");
    test_valid_body();
    test_hard_limit();
    test_buffer_contract();
    if (g_fail == 0) {
        std::printf("  PASS (todos)\n");
        return 0;
    }
    std::printf("  %d checagens falharam\n", g_fail);
    return 1;
}
