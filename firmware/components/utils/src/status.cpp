#include "utils/status.hpp"

namespace nova {
namespace utils {

const char* to_string(Status s) {
    switch (s) {
        case Status::kOk:          return "ok";
        case Status::kTimeout:     return "timeout";
        case Status::kNetworkDown: return "network-down";
        case Status::kBusy:        return "busy";
        case Status::kHttpError:   return "http-error";
        case Status::kTooLarge:    return "too-large";
        case Status::kMalformed:   return "malformed";
        case Status::kStale:       return "stale";
        case Status::kNoMemory:    return "no-memory";
        case Status::kNotFound:    return "not-found";
        case Status::kInvalidArg:  return "invalid-arg";
        case Status::kInternal:    return "internal";
    }
    return "?";
}

bool is_transient(Status s) {
    switch (s) {
        // Vale repetir mais tarde: a causa é externa e passageira.
        case Status::kTimeout:
        case Status::kNetworkDown:
        case Status::kBusy:
        case Status::kNoMemory:
            return true;
        // NÃO vale repetir: repetir não muda o resultado e só gasta banda.
        // kHttpError inclui 4xx (pedido errado) e 5xx; tratar tudo como
        // permanente é o conservador — o orquestrador decide reagendar pelo
        // intervalo normal do domínio, não por retry imediato.
        case Status::kOk:
        case Status::kHttpError:
        case Status::kTooLarge:
        case Status::kMalformed:
        case Status::kStale:
        case Status::kNotFound:
        case Status::kInvalidArg:
        case Status::kInternal:
            return false;
    }
    return false;
}

}  // namespace utils
}  // namespace nova
