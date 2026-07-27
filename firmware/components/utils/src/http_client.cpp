#include "utils/http_client.hpp"

#include <cstring>

namespace nova {
namespace utils {

BoundedHttpBody::BoundedHttpBody(uint8_t* storage, size_t capacity)
    : storage_(storage), capacity_(capacity) {
    reset();
}

Status BoundedHttpBody::reset() {
    size_ = 0;
    status_ = storage_ != nullptr && capacity_ >= kHttpBodyMaxBytes ? Status::kOk : Status::kNoMemory;
    return status_;
}

Status BoundedHttpBody::append(const uint8_t* chunk, size_t bytes) {
    if (status_ != Status::kOk) {
        return status_;
    }
    if (chunk == nullptr && bytes != 0) {
        status_ = Status::kInvalidArg;
        return status_;
    }
    if (bytes > kHttpBodyMaxBytes - size_) {
        // Descarta o parcial: não existe caminho que permita ao parser aceitar
        // uma resposta depois que ela excedeu o contrato físico de 48 KiB.
        size_ = 0;
        status_ = Status::kTooLarge;
        return status_;
    }
    if (bytes != 0) {
        std::memcpy(storage_ + size_, chunk, bytes);
        size_ += bytes;
    }
    return Status::kOk;
}

Result<HttpResponse> BoundedHttpBody::finish(uint16_t status_code) const {
    if (status_ != Status::kOk) {
        return Result<HttpResponse>::fail(status_);
    }
    return Result<HttpResponse>::ok(HttpResponse{status_code, storage_, size_});
}

}  // namespace utils
}  // namespace nova
