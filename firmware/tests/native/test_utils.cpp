// Testes de utils/ (Status e Result<T>). Sem placa, sem alocação.
#include <cstdio>
#include <cstring>

#include "utils/result.hpp"
#include "utils/status.hpp"

namespace {
int g_fail = 0;

void check(bool cond, const char* what) {
    if (!cond) {
        std::printf("  FAIL: %s\n", what);
        ++g_fail;
    }
}

void test_status_strings() {
    using namespace nova::utils;
    // Todo valor do enum tem texto próprio: um "?" aqui viraria log ilegível
    // justamente no caso raro, que é quando o log importa.
    check(std::strcmp(to_string(Status::kOk), "ok") == 0, "to_string(kOk)");
    check(std::strcmp(to_string(Status::kTooLarge), "too-large") == 0, "to_string(kTooLarge)");
    check(std::strcmp(to_string(Status::kMalformed), "malformed") == 0, "to_string(kMalformed)");

    const Status all[] = {
        Status::kOk,        Status::kTimeout,  Status::kNetworkDown, Status::kBusy,
        Status::kHttpError, Status::kTooLarge, Status::kMalformed,   Status::kStale,
        Status::kNoMemory,  Status::kNotFound, Status::kInvalidArg,  Status::kInternal,
    };
    bool todos_nomeados = true;
    for (Status s : all) {
        if (std::strcmp(to_string(s), "?") == 0) {
            todos_nomeados = false;
        }
    }
    check(todos_nomeados, "todo Status tem texto (nenhum cai em '?')");
}

void test_transient() {
    using namespace nova::utils;
    // Repetir faz sentido: causa externa e passageira.
    check(is_transient(Status::kTimeout), "timeout e transitorio");
    check(is_transient(Status::kNetworkDown), "network-down e transitorio");
    check(is_transient(Status::kBusy), "busy e transitorio");

    // Repetir NÃO faz sentido: o payload mudou de forma, ou o pedido está
    // errado. Retry aqui só gastaria banda e cota do provedor.
    check(!is_transient(Status::kMalformed), "malformed NAO e transitorio");
    check(!is_transient(Status::kTooLarge), "too-large NAO e transitorio");
    check(!is_transient(Status::kInvalidArg), "invalid-arg NAO e transitorio");
    check(!is_transient(Status::kOk), "ok nao e falha, logo nao e transitorio");
}

void test_result_value() {
    using namespace nova::utils;
    Result<int> good = Result<int>::ok(42);
    check(good.is_ok(), "Result::ok e ok");
    check(good.status() == Status::kOk, "status de sucesso e kOk");
    check(good.value() == 42, "value() devolve o valor");
    check(good.value_or(-1) == 42, "value_or devolve o valor quando ok");

    Result<int> bad = Result<int>::fail(Status::kTimeout);
    check(!bad.is_ok(), "Result::fail nao e ok");
    check(bad.status() == Status::kTimeout, "status carrega o MOTIVO da falha");
    check(bad.value_or(-1) == -1, "value_or devolve o fallback quando falho");
}

void test_result_void() {
    using namespace nova::utils;
    Result<void> good = Result<void>::ok();
    check(good.is_ok(), "Result<void>::ok e ok");

    Result<void> bad = Result<void>::fail(Status::kNoMemory);
    check(!bad.is_ok(), "Result<void>::fail nao e ok");
    check(bad.status() == Status::kNoMemory, "Result<void> preserva o motivo");
}

// Tipo não-trivial: prova que Result move o valor e não exige alocação nossa.
struct Payload {
    int a_ = 0;
    char tag_[8] = {};
};

// Provider models podem exigir dados obrigatórios. Result<T>::fail() não pode
// exigir um construtor padrão que o domínio não precisa.
struct RequiredValue {
    explicit RequiredValue(int number) : number_(number) {}
    RequiredValue() = delete;

    int number_;
};

void test_result_struct() {
    using namespace nova::utils;
    Payload p;
    p.a_ = 7;
    p.tag_[0] = 'x';

    Result<Payload> r = Result<Payload>::ok(p);
    check(r.is_ok() && r.value().a_ == 7 && r.value().tag_[0] == 'x',
          "Result carrega struct por valor");

    Result<Payload> f = Result<Payload>::fail(Status::kMalformed);
    check(!f.is_ok(), "Result<struct> falho");
    check(f.value_or(Payload{}).a_ == 0, "value_or devolve o fallback em struct");
}

void test_result_without_default_constructor() {
    using namespace nova::utils;
    Result<RequiredValue> good = Result<RequiredValue>::ok(RequiredValue(9));
    check(good.is_ok() && good.value().number_ == 9,
          "Result aceita tipo sem construtor padrao no sucesso");

    Result<RequiredValue> bad = Result<RequiredValue>::fail(Status::kMalformed);
    check(!bad.is_ok() && bad.status() == Status::kMalformed,
          "Result falho nao constroi tipo sem construtor padrao");
}
}  // namespace

int main() {
    std::printf("utils tests:\n");
    test_status_strings();
    test_transient();
    test_result_value();
    test_result_void();
    test_result_struct();
    test_result_without_default_constructor();
    if (g_fail == 0) {
        std::printf("  PASS (todos)\n");
        return 0;
    }
    std::printf("  %d checagens falharam\n", g_fail);
    return 1;
}
