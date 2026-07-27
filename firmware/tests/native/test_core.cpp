// Testes do núcleo (docs/TESTING.md, critério de saída da Onda A: "testes de
// host cobrindo estado, eventos, orquestrador e fila").
//
// Rodam sem placa: `core` recebe o lock por injeção, então aqui usamos um lock
// instrumentado que CONTA lock/unlock — assim o teste também prova que os
// acessores realmente serializam, em vez de confiar em comentário (ADR-008).
#include <cstdio>

#include "core/action_queue.hpp"
#include "core/event_bus.hpp"
#include "core/state_store.hpp"

namespace {
int g_fail = 0;

void check(bool cond, const char* what) {
    if (!cond) {
        std::printf("  FAIL: %s\n", what);
        ++g_fail;
    }
}

// Lock que conta uso e detecta desbalanceamento (unlock sem lock, ou saída de
// função sem unlock).
class CountingLock : public nova::core::ILock {
public:
    void lock() override {
        ++locks_;
        ++depth_;
        if (depth_ > 1) {
            ++reentrant_;  // lock não é recursivo: isso seria deadlock no alvo
        }
    }
    void unlock() override {
        --depth_;
        ++unlocks_;
    }
    int locks_ = 0;
    int unlocks_ = 0;
    int depth_ = 0;
    int reentrant_ = 0;
};

// ── EventBus ─────────────────────────────────────────────────────────────────
int g_calls = 0;
nova::models::Event g_last = nova::models::Event::kCount;

void counting_handler(nova::models::Event e, void*) {
    ++g_calls;
    g_last = e;
}

nova::core::EventBus* g_bus = nullptr;
void reentrant_handler(nova::models::Event, void*) {
    // Tenta publicar de dentro do despacho: deve ser rejeitado, não recursionar.
    g_bus->publish(nova::models::Event::kNetworkChanged);
}

void test_event_bus() {
    using namespace nova;
    core::EventBus bus;
    g_calls = 0;

    check(bus.subscribe(counting_handler, nullptr), "subscribe aceita handler");
    check(!bus.subscribe(nullptr, nullptr), "subscribe rejeita handler nulo");
    check(bus.subscriber_count() == 1, "1 assinante");

    check(bus.publish(models::Event::kClockChanged) == 1, "publish chama 1 handler");
    check(g_calls == 1 && g_last == models::Event::kClockChanged, "handler recebeu o evento");

    // Lotação: aceita até kMaxSubscribers e recusa além, sem estourar.
    core::EventBus full;
    for (size_t i = 0; i < core::EventBus::kMaxSubscribers; ++i) {
        check(full.subscribe(counting_handler, nullptr), "subscribe dentro da capacidade");
    }
    check(!full.subscribe(counting_handler, nullptr), "subscribe recusa acima da capacidade");

    // Reentrância: rejeitada e CONTADA, sem recursão infinita.
    core::EventBus re;
    g_bus = &re;
    re.subscribe(reentrant_handler, nullptr);
    re.publish(models::Event::kClockChanged);
    check(re.rejected_reentrant() == 1, "publish reentrante e rejeitado e contado");
}

// ── StateStore ───────────────────────────────────────────────────────────────
void test_state_store() {
    using namespace nova;
    CountingLock lk;
    core::StateStore st(lk);

    check(st.set_clock(10, 30, true), "set_clock com valor novo => true");
    check(!st.set_clock(10, 30, true), "set_clock com valor IGUAL => false (dedup)");
    check(st.set_clock(10, 31, true), "set_clock com minuto novo => true");

    models::ClockState c = st.clock();
    check(c.hour_ == 10 && c.minute_ == 31 && c.valid_, "clock() devolve o valor gravado");

    check(st.set_network(models::NetworkState::kUp), "set_network novo => true");
    check(!st.set_network(models::NetworkState::kUp), "set_network igual => false (dedup)");
    check(st.network() == models::NetworkState::kUp, "network() devolve o valor");

    // Prova que os acessores serializam de verdade e não vazam lock.
    check(lk.locks_ == lk.unlocks_, "todo lock teve unlock (sem vazamento)");
    check(lk.locks_ > 0, "acessores realmente usam o lock");
    check(lk.depth_ == 0, "profundidade zerada ao fim");
    check(lk.reentrant_ == 0, "nenhum lock aninhado (seria deadlock no alvo)");
}

// ── ActionQueue ──────────────────────────────────────────────────────────────
void test_action_queue() {
    using namespace nova;
    CountingLock lk;
    core::ActionQueue q(lk);
    core::Action a = core::Action::kCount;

    check(q.empty(), "fila nasce vazia");
    check(!q.pop(a), "pop em fila vazia => false");

    check(q.push(core::Action::kRefreshClock), "push aceita");
    check(q.size() == 1, "size = 1");
    check(q.pop(a) && a == core::Action::kRefreshClock, "pop devolve o que entrou");
    check(q.empty(), "fila vazia depois do pop");

    // FIFO e wrap-around do buffer circular.
    for (size_t i = 0; i < core::ActionQueue::kCapacity; ++i) {
        check(q.push(i % 2 == 0 ? core::Action::kRefreshClock : core::Action::kRetryNetwork),
              "push ate lotar");
    }
    check(q.size() == core::ActionQueue::kCapacity, "fila cheia na capacidade");

    // Overflow: recusa E CONTA — nunca descarte silencioso (ADR-008).
    check(!q.push(core::Action::kRefreshClock), "push em fila cheia => false");
    check(q.overflows() == 1, "overflow contado");
    check(!q.push(core::Action::kRefreshClock), "segundo overflow");
    check(q.overflows() == 2, "overflows acumulam");

    // Ordem preservada.
    bool order_ok = true;
    for (size_t i = 0; i < core::ActionQueue::kCapacity; ++i) {
        core::Action expected =
            (i % 2 == 0) ? core::Action::kRefreshClock : core::Action::kRetryNetwork;
        if (!q.pop(a) || a != expected) {
            order_ok = false;
        }
    }
    check(order_ok, "FIFO preservado, inclusive com wrap-around");
    check(q.empty(), "fila drenada");

    check(lk.locks_ == lk.unlocks_, "fila: todo lock teve unlock");
    check(lk.reentrant_ == 0, "fila: nenhum lock aninhado");
}

// ── Máscara de eventos ───────────────────────────────────────────────────────
void test_event_mask() {
    using namespace nova::models;
    check(mask_of(Event::kClockChanged) == 1u, "bit 0 = ClockChanged");
    check(mask_of(Event::kNetworkChanged) == 2u, "bit 1 = NetworkChanged");
    EventMask m = mask_of(Event::kClockChanged) | mask_of(Event::kResourceWarning);
    check((m & mask_of(Event::kNetworkChanged)) == 0, "mascara nao pega evento ausente");
    check((m & mask_of(Event::kResourceWarning)) != 0, "mascara pega evento presente");
}
}  // namespace

int main() {
    std::printf("core tests:\n");
    test_event_bus();
    test_state_store();
    test_action_queue();
    test_event_mask();
    if (g_fail == 0) {
        std::printf("  PASS (todos)\n");
        return 0;
    }
    std::printf("  %d checagens falharam\n", g_fail);
    return 1;
}
