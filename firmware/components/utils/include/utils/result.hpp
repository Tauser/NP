// Result<T> — valor OU falha, sem exceções e sem alocação.
//
// Embarcado não tem exceção (custo de binário e de determinismo), e devolver
// código de erro por parâmetro-saída é o padrão que produz o bug clássico:
// ninguém checa. Aqui o valor só sai de dentro de um objeto que carrega o
// status junto, e ler o valor de um Result falho é erro detectável.
//
// NOMES: `ok()` é a FÁBRICA (constrói um sucesso) e `is_ok()` é a CONSULTA.
// Usar `ok()` para as duas coisas colide em `Result<void>`, onde as assinaturas
// ficariam idênticas.
//
// PURO e header-only: `utils` não depende de nada.
#pragma once

#include <cassert>
#include <optional>
#include <utility>

#include "utils/status.hpp"

namespace nova {
namespace utils {

template <typename T>
class Result {
public:
    // Fábricas nomeadas em vez de construtores: `Result<int>::fail(...)` diz o
    // que é no ponto de uso; um construtor implícito não diria.
    static Result ok(T value) { return Result(std::move(value), Status::kOk); }
    static Result fail(Status s) {
        assert(s != Status::kOk && "fail(kOk) e contradicao");
        return Result(std::nullopt, s);
    }

    bool is_ok() const { return status_ == Status::kOk; }
    Status status() const { return status_; }

    // Ler o valor de um Result falho é BUG do chamador. Em dev o assert aponta
    // o local exato. Em release a pré-condição continua a mesma; prefira
    // `value_or()` quando a falha for esperada.
    const T& value() const {
        assert(is_ok() && "value() em Result falho — cheque is_ok() antes");
        return *value_;
    }

    T value_or(T fallback) const { return is_ok() ? *value_ : fallback; }

private:
    Result(T v, Status s) : value_(std::move(v)), status_(s) {}
    Result(std::nullopt_t, Status s) : value_(std::nullopt), status_(s) {}

    std::optional<T> value_;
    Status status_;
};

// Especialização para operações sem valor de retorno: "deu certo ou não".
// Existe para que uma função sem resultado use o MESMO vocabulário das que têm,
// em vez de devolver `bool` e perder o motivo da falha.
template <>
class Result<void> {
public:
    static Result ok() { return Result(Status::kOk); }
    static Result fail(Status s) {
        assert(s != Status::kOk && "fail(kOk) e contradicao");
        return Result(s);
    }

    bool is_ok() const { return status_ == Status::kOk; }
    Status status() const { return status_; }

private:
    explicit Result(Status s) : status_(s) {}
    Status status_;
};

}  // namespace utils
}  // namespace nova
