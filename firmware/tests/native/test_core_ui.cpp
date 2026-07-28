// Testes de fila, despacho e pump. Separado de test_core.cpp para manter cada
// unidade dentro do limite de revisão de ARCHITECTURE.md §11.
#include <cstdio>

#include "core/action_queue.hpp"
#include "core/event_bus.hpp"
#include "core/pump.hpp"
#include "core/state_store.hpp"
#include "core/ui_dispatcher.hpp"

namespace {
int g_fail = 0;

void check(bool cond, const char* what) {
    if (!cond) {
        std::printf("  FAIL: %s\n", what);
        ++g_fail;
    }
}

class CountingLock : public nova::core::ILock {
public:
    void lock() override { ++locks_; }
    void unlock() override { ++unlocks_; }
    int locks_ = 0;
    int unlocks_ = 0;
};

void test_action_queue() {
    using namespace nova;
    CountingLock lock;
    core::ActionQueue queue(lock);
    core::Action action = core::Action::kCount;
    check(queue.empty() && !queue.pop(action), "fila nasce vazia");
    check(queue.push(core::Action::kRefreshClock), "push aceita");
    check(queue.pop(action) && action == core::Action::kRefreshClock, "pop devolve o que entrou");

    for (size_t i = 0; i < core::ActionQueue::kCapacity; ++i) {
        const core::Action expected = i % 2 == 0 ? core::Action::kRefreshClock
                                                   : core::Action::kRetryNetwork;
        check(queue.push(expected), "push ate lotar");
    }
    check(!queue.push(core::Action::kRefreshClock), "overflow recusa entrada");
    check(queue.overflows() == 1, "overflow contado");
    for (size_t i = 0; i < core::ActionQueue::kCapacity; ++i) {
        const core::Action expected = i % 2 == 0 ? core::Action::kRefreshClock
                                                   : core::Action::kRetryNetwork;
        check(queue.pop(action) && action == expected, "FIFO preservado no wrap-around");
    }
    check(queue.empty() && lock.locks_ == lock.unlocks_, "fila drenada com locks balanceados");
}

struct FakeScreen {
    int updates_ = 0;
};

void fake_update(void* context) { ++static_cast<FakeScreen*>(context)->updates_; }

void test_ui_dispatcher() {
    using namespace nova;
    core::UiDispatcher dispatcher;
    FakeScreen clock;
    FakeScreen status;
    const size_t clock_id = dispatcher.register_target(models::mask_of(models::Event::kClockChanged),
                                                        fake_update, &clock);
    dispatcher.register_target(models::mask_of(models::Event::kClockChanged) |
                                   models::mask_of(models::Event::kNetworkChanged),
                               fake_update, &status);
    core::UiDispatcher::on_event(models::Event::kNetworkChanged, &dispatcher);
    check(clock_id != core::UiDispatcher::kInvalidTarget && dispatcher.dispatch() == 1,
          "evento invalida somente a tela interessada");
    check(clock.updates_ == 0 && status.updates_ == 1, "mascara respeitada");

    core::UiDispatcher::on_event(models::Event::kClockChanged, &dispatcher);
    core::UiDispatcher::on_event(models::Event::kNetworkChanged, &dispatcher);
    check(dispatcher.dispatch() == 2 && clock.updates_ == 1 && status.updates_ == 2,
          "coalescing atualiza cada tela uma vez por ciclo");
    core::UiDispatcher hidden_dispatcher;
    FakeScreen hidden;
    const size_t hidden_id = hidden_dispatcher.register_target(
        models::mask_of(models::Event::kClockChanged), fake_update, &hidden);
    hidden_dispatcher.set_visible(hidden_id, false);
    core::UiDispatcher::on_event(models::Event::kClockChanged, &hidden_dispatcher);
    check(hidden_dispatcher.dispatch() == 0 && hidden_dispatcher.is_dirty(hidden_id),
          "tela invisivel fica suja");
    hidden_dispatcher.set_visible(hidden_id, true);
    check(hidden.updates_ == 1 && !hidden_dispatcher.is_dirty(hidden_id),
          "on_enter repinta tela suja");
}

void test_pump_end_to_end() {
    using namespace nova;
    CountingLock lock;
    core::StateStore store(lock);
    core::EventBus bus;
    core::UiDispatcher dispatcher;
    FakeScreen screen;
    dispatcher.register_target(models::mask_of(models::Event::kClockChanged), fake_update, &screen);
    check(bus.subscribe(core::UiDispatcher::on_event, &dispatcher), "dispatcher assina o bus");
    for (uint8_t minute = 0; minute <= 30; ++minute) {
        store.set_clock({0, 8, minute, models::ClockSource::kRtc, true, false});
    }
    check(core::pump_once(store, bus, dispatcher) == 1 && screen.updates_ == 1,
          "pump coalesce 31 mutacoes em uma invalidacao");
    check(store.clock().minute_ == 30 && core::pump_once(store, bus, dispatcher) == 0,
          "estado final e tick ocioso corretos");
    check(lock.locks_ == lock.unlocks_, "pump balanceia locks");
}

void test_event_mask() {
    using namespace nova::models;
    const EventMask mask = mask_of(Event::kClockChanged) | mask_of(Event::kResourceWarning);
    check(mask_of(Event::kClockChanged) == 1U && (mask & mask_of(Event::kNetworkChanged)) == 0,
          "mascara preserva bits esperados");
}
}  // namespace

int main() {
    std::printf("core UI tests:\n");
    test_action_queue();
    test_ui_dispatcher();
    test_pump_end_to_end();
    test_event_mask();
    if (g_fail == 0) {
        std::printf("  PASS (todos)\n");
        return 0;
    }
    return 1;
}
