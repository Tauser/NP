// Testes do núcleo (docs/TESTING.md, critério de saída da Onda A: "testes de
// host cobrindo estado, eventos, orquestrador e fila").
//
// Rodam sem placa: `core` recebe o lock por injeção, então aqui usamos um lock
// instrumentado que CONTA lock/unlock — assim o teste também prova que os
// acessores realmente serializam, em vez de confiar em comentário (ADR-008).
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
// Espião de eventos: registra o que foi publicado, para provar "evento correto"
// e "nenhuma publicação quando o valor não mudou".
struct Spy {
    int count_ = 0;
    nova::models::Event last_ = nova::models::Event::kCount;
};
Spy g_spy;

void spy_handler(nova::models::Event e, void* ctx) {
    Spy* s = static_cast<Spy*>(ctx);
    ++s->count_;
    s->last_ = e;
}

// Simula o que a `app_loop` faz no tick: drena a máscara e publica cada bit.
// É o ÚNICO ponto do sistema que chama publish().
size_t drain_and_publish(nova::core::StateStore& st, nova::core::EventBus& bus) {
    using namespace nova::models;
    const EventMask m = st.take_pending_events();
    size_t published = 0;
    for (uint8_t i = 0; i < kEventCount; ++i) {
        const Event e = static_cast<Event>(i);
        if ((m & mask_of(e)) != 0) {
            bus.publish(e);
            ++published;
        }
    }
    return published;
}

// Handler que LÊ o estado durante o despacho. Como a publicação acontece fora
// de qualquer região crítica do StateStore, isto não pode aninhar lock.
nova::core::StateStore* g_store_for_reentrancy = nullptr;
void reading_handler(nova::models::Event, void*) {
    if (g_store_for_reentrancy != nullptr) {
        (void)g_store_for_reentrancy->clock();
    }
}

void test_state_store() {
    using namespace nova;
    CountingLock lk;
    core::EventBus bus;
    g_spy = Spy{};
    bus.subscribe(spy_handler, &g_spy);
    core::StateStore st(lk);

    // set_clock: estado correto + fato registrado, mas SEM publicar sozinho.
    check(st.set_clock({0, 10, 30, models::ClockSource::kRtc, true, false}),
          "set_clock com valor novo => true");
    check(g_spy.count_ == 0, "setter NAO publica sozinho (so a app_loop publica)");
    check(drain_and_publish(st, bus) == 1, "drenagem publicou 1 evento");
    check(g_spy.last_ == models::Event::kClockChanged, "evento publicado e ClockChanged");

    // Valor idêntico: NÃO muta e NÃO registra fato — nada a drenar.
    check(!st.set_clock({0, 10, 30, models::ClockSource::kRtc, true, false}),
          "set_clock com valor IGUAL => false");
    check(drain_and_publish(st, bus) == 0, "valor igual NAO gerou evento (dedup)");
    check(g_spy.count_ == 1, "contagem de eventos inalterada");

    check(st.set_clock({0, 10, 31, models::ClockSource::kRtc, true, false}),
          "set_clock com minuto novo => true");
    check(drain_and_publish(st, bus) == 1, "mudanca real gerou evento de novo");
    models::ClockState c = st.clock();
    check(c.hour_ == 10 && c.minute_ == 31 && c.valid_, "clock() devolve o valor gravado");

    // set_network: mesmo contrato, evento próprio.
    check(st.set_network(models::NetworkState::kUp), "set_network novo => true");
    check(drain_and_publish(st, bus) == 1, "set_network gerou 1 evento");
    check(g_spy.last_ == models::Event::kNetworkChanged, "evento e NetworkChanged");
    check(!st.set_network(models::NetworkState::kUp), "set_network igual => false");
    check(drain_and_publish(st, bus) == 0, "set_network igual NAO gerou evento");
    check(st.network() == models::NetworkState::kUp, "network() devolve o valor");

    const models::WifiSetupState setup{42, models::WifiSetupPhase::kConnected, true};
    check(st.set_wifi_setup(setup), "set_wifi_setup novo => true");
    check(drain_and_publish(st, bus) == 1, "set_wifi_setup gerou 1 evento");
    check(g_spy.last_ == models::Event::kWifiSetupChanged, "evento e WifiSetupChanged");
    check(!st.set_wifi_setup(setup), "set_wifi_setup igual => false");
    check(drain_and_publish(st, bus) == 0, "setup igual nao gerou evento");
    check(st.wifi_setup().has_saved_credentials_, "wifi_setup() devolve o valor");

    const models::WeatherState weather{100, 246, 231, 124, 3,
                                       models::WeatherSource::kLive, true, true, false};
    check(st.set_weather(weather), "set_weather novo => true");
    check(drain_and_publish(st, bus) == 1, "set_weather gerou 1 evento");
    check(g_spy.last_ == models::Event::kWeatherChanged, "evento e WeatherChanged");
    check(!st.set_weather(weather), "set_weather igual => false");
    check(st.weather().temperature_deci_c_ == 246 && !st.weather().stale_,
          "weather() devolve o valor");

    // Prova que os acessores serializam e não vazam lock.
    check(lk.locks_ == lk.unlocks_, "todo lock teve unlock (sem vazamento)");
    check(lk.locks_ > 0, "acessores realmente usam o lock");
    check(lk.depth_ == 0, "profundidade zerada ao fim");
    check(lk.reentrant_ == 0, "nenhum lock aninhado (seria deadlock no alvo)");
}

// COALESCING: é o ganho de mover a publicação para a drenagem. Muitas mutações
// entre dois ticks viram UM evento por tipo — não N. Ataca direto o custo de
// banda MSPI (RESOURCE-BUDGET §1.1).
void test_coalescing() {
    using namespace nova;
    CountingLock lk;
    core::EventBus bus;
    g_spy = Spy{};
    bus.subscribe(spy_handler, &g_spy);
    core::StateStore st(lk);

    for (uint8_t m = 0; m < 20; ++m) {
        st.set_clock({0, 9, m, models::ClockSource::kRtc, true, false});  // 20 mutações REAIS
    }
    check(drain_and_publish(st, bus) == 1, "20 mutacoes de relogio => 1 evento");
    check(g_spy.count_ == 1, "handler chamado uma unica vez");
    check(st.clock().minute_ == 19, "estado reflete a ULTIMA mutacao");

    // Dois domínios distintos coalescem separadamente: 1 evento cada.
    // (kUp difere do default kDown; usar kDown aqui seria um não-evento.)
    st.set_clock({0, 10, 0, models::ClockSource::kRtc, true, false});
    st.set_network(models::NetworkState::kUp);
    check(drain_and_publish(st, bus) == 2, "dominios distintos => 1 evento cada");

    // Drenar de novo sem mutar: nada pendente.
    check(drain_and_publish(st, bus) == 0, "drenagem apos drenagem nao repete evento");
}

// A publicação acontece fora de qualquer região crítica do StateStore, então um
// handler pode ler o estado sem aninhar lock.
void test_publish_outside_lock() {
    using namespace nova;
    CountingLock lk;
    core::EventBus bus;
    core::StateStore st(lk);
    g_store_for_reentrancy = &st;
    bus.subscribe(reading_handler, nullptr);

    st.set_clock({0, 7, 45, models::ClockSource::kRtc, true, false});
    drain_and_publish(st, bus);

    check(lk.reentrant_ == 0, "handler leu o estado SEM aninhar lock");
    check(lk.locks_ == lk.unlocks_, "locks balanceados apos publish com leitura no handler");
    g_store_for_reentrancy = nullptr;
}

}  // namespace

int main() {
    std::printf("core tests:\n");
    test_event_bus();
    test_state_store();
    test_coalescing();
    test_publish_outside_lock();
    if (g_fail == 0) {
        std::printf("  PASS (todos)\n");
        return 0;
    }
    std::printf("  %d checagens falharam\n", g_fail);
    return 1;
}
