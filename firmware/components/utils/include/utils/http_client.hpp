// Contrato HTTP puro: o transporte real fica na borda, o teto fica aqui.
//
// O corpo é fornecido pelo futuro NetworkWorker e NUNCA alocado por esta API.
// Ele deve viver em SRAM interna, ter 48 KiB e sobreviver até o provider ter
// consumido a resposta (RESOURCE-BUDGET §2). Assim uma resposta truncada não
// chega ao parser como se fosse válida.
#pragma once

#include <cstddef>
#include <cstdint>

#include "utils/result.hpp"

namespace nova {
namespace utils {

constexpr size_t kHttpBodyMaxBytes = 48 * 1024;

struct HttpRequest {
    const char* url = nullptr;
    uint32_t timeout_ms = 0;
};

struct HttpResponse {
    uint16_t status_code = 0;
    const uint8_t* body = nullptr;
    size_t body_size = 0;
};

class BoundedHttpBody {
public:
    // `storage` precisa ter ao menos kHttpBodyMaxBytes. Menos que isso é erro
    // de orçamento (kNoMemory), nunca um teto implícito menor.
    BoundedHttpBody(uint8_t* storage, size_t capacity);

    Status reset();
    Status append(const uint8_t* chunk, size_t bytes);
    Result<HttpResponse> finish(uint16_t status_code) const;

    size_t size() const { return size_; }
    Status status() const { return status_; }

private:
    uint8_t* storage_ = nullptr;
    size_t capacity_ = 0;
    size_t size_ = 0;
    Status status_ = Status::kNoMemory;
};

// Providers dependem desta interface, nunca do cliente ESP-IDF concreto. A
// implementação futura roda somente no NetworkWorker e deve serializar toda
// chamada, mesmo que algum provider tente contornar o RequestOrchestrator.
class IHttpClient {
public:
    virtual ~IHttpClient() = default;
    virtual Result<HttpResponse> get(const HttpRequest& request, BoundedHttpBody& body) = 0;
};

}  // namespace utils
}  // namespace nova
