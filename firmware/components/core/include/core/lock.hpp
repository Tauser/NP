// Exclusão mútua abstrata para o núcleo (docs/ARCHITECTURE.md §5, ADR-008).
//
// `core` não conhece FreeRTOS: o lock é INJETADO. No firmware vem da HAL; nos
// testes de host vem um lock de mentira. É o que permite testar todo o núcleo
// sem placa.
#pragma once

namespace nova {
namespace core {

class ILock {
public:
    virtual ~ILock() = default;
    virtual void lock() = 0;
    virtual void unlock() = 0;
};

// Lock nulo: uso single-thread (testes e boot antes de as tasks existirem).
class NullLock : public ILock {
public:
    void lock() override {}
    void unlock() override {}
};

// RAII sobre uma ILock injetada. Garante unlock em todo caminho de saída,
// inclusive em `return` no meio da função.
class LockGuard {
public:
    explicit LockGuard(ILock& l) : l_(l) { l_.lock(); }
    ~LockGuard() { l_.unlock(); }
    LockGuard(const LockGuard&) = delete;
    LockGuard& operator=(const LockGuard&) = delete;

private:
    ILock& l_;
};

}  // namespace core
}  // namespace nova
