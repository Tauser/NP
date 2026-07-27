// RequestOrchestrator — política pura para todo tráfego externo.
//
// O worker futuro executa HTTP; esta classe decide QUANDO e QUAL domínio pode
// começar. Assim, 1 HTTPS por vez não depende de cada provider lembrar da
// regra (RESOURCE-BUDGET §2), e a política continua testável sem IDF ou rede.
#pragma once

#include <cstddef>
#include <cstdint>

#include "core/lock.hpp"
#include "utils/result.hpp"

namespace nova {
namespace core {

using RequestId = uint8_t;
constexpr RequestId kInvalidRequestId = UINT8_MAX;

enum class CircuitState : uint8_t {
    kClosed = 0,
    kOpen,
    kHalfOpen,
};

struct RequestPolicy {
    uint32_t min_interval_ms = 0;
    uint32_t initial_backoff_ms = 0;
    uint32_t max_backoff_ms = 0;
    uint8_t failures_to_open = 0;
    uint8_t priority = 0;        // menor número vence
    uint8_t jitter_percent = 0;  // 0..50, aplicado simetricamente ao backoff
};

struct RequestLease {
    RequestId id = kInvalidRequestId;
    uint32_t sequence = 0;
};

// Função de jitter injetada: devolve valor em [0, upper_inclusive]. O produto
// pode ligá-la a uma fonte aleatória; o host usa a implementação determinista.
using JitterFn = uint32_t (*)(uint32_t upper_inclusive, void* ctx);

class RequestOrchestrator {
public:
    static constexpr size_t kMaxRequests = 8;
    static constexpr uint32_t kDefaultGlobalGapMs = 1000;

    explicit RequestOrchestrator(ILock& lock, uint32_t global_gap_ms = kDefaultGlobalGapMs,
                                 JitterFn jitter = nullptr, void* jitter_ctx = nullptr);

    // Registra política, mas deixa o domínio desligado até `set_enabled`.
    // Configuração inválida e capacidade esgotada retornam o motivo explícito.
    utils::Result<RequestId> register_request(RequestPolicy policy);

    // O primeiro registro já é elegível no tempo zero; depois de sucesso a
    // própria política agenda a próxima. Alternar habilitação NÃO reinicia
    // prazo: do contrário um caller poderia furar backoff ou breaker.
    // Desabilitar nunca cancela uma lease em execução, apenas impede seleção.
    bool set_enabled(RequestId id, bool enabled);

    // Entrega no máximo UMA lease global. O worker deve chamar `complete()`
    // exatamente uma vez; até lá qualquer nova aquisição recebe kBusy.
    utils::Result<RequestLease> take_next(uint64_t now_ms);
    bool complete(RequestLease lease, utils::Status result, uint64_t now_ms);

    bool has_active_request() const;
    CircuitState circuit_state(RequestId id) const;

private:
    struct Slot {
        RequestPolicy policy{};
        uint64_t next_due_ms = 0;
        uint32_t sequence = 0;
        uint8_t failures = 0;
        bool registered = false;
        bool enabled = false;
        CircuitState circuit = CircuitState::kClosed;
    };

    static uint32_t no_jitter(uint32_t upper_inclusive, void* ctx);
    static bool is_valid_policy(const RequestPolicy& policy);
    static uint64_t after(uint64_t now_ms, uint32_t delay_ms);

    uint32_t retry_delay(const Slot& slot) const;
    uint32_t jittered_delay(uint32_t delay_ms, uint8_t percent) const;
    bool is_valid_id(RequestId id) const;

    ILock& lock_;
    Slot slots_[kMaxRequests] = {};
    uint32_t global_gap_ms_ = kDefaultGlobalGapMs;
    uint64_t next_global_start_ms_ = 0;
    RequestId active_id_ = kInvalidRequestId;
    uint32_t active_sequence_ = 0;
    JitterFn jitter_ = no_jitter;
    void* jitter_ctx_ = nullptr;
};

}  // namespace core
}  // namespace nova
