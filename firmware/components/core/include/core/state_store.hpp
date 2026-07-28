// StateStore — dono único do AppState (docs/ARCHITECTURE.md §6, ADR-006).
//
// PROPRIEDADE (ADR-008): o estado é mutado pela `app_loop` e pela `net_worker`;
// é LIDO pela task de UI. Portanto TODO acesso passa por este objeto, que
// serializa com um lock injetado. Não há campo público, e não existe getter que
// devolva `AppState` inteiro — de propósito, ver abaixo.
//
// POR QUE NÃO HÁ `const AppState& state()` (ADR-011):
// a rotina de render leria a struct inteira e a copiaria para a pilha da task de
// UI. Isso já causou *stack protection fault* em campo
// (PATRIMONIO-TECNICO §5.1). Os acessores são GRANULARES por domínio e devolvem
// tipos pequenos por valor. Se um acessor novo começar a devolver algo grande,
// o custo aparece na marca d'água da pilha (RESOURCE-BUDGET §2.1), medida pela
// instrumentação permanente.
//
// O lock é injetado (não é criado aqui) para manter `core` puro e testável no
// host: o teste usa um lock de mentira, o firmware usa o da HAL.
#pragma once

#include <cstdint>

#include "core/lock.hpp"
#include "models/app_state.hpp"
#include "models/events.hpp"

namespace nova {
namespace core {

class StateStore {
public:
    explicit StateStore(ILock& lock) : lock_(lock) {}

    // ── Mutação: muta e REGISTRA o fato; não publica ─────────────────────────
    // O setter é o único caminho de escrita. Ele NÃO chama o EventBus.
    //
    // POR QUE NÃO PUBLICA AQUI (concorrência): estes setters são chamados pela
    // `app_loop` E pela `net_worker` (ARCHITECTURE §5). O EventBus é
    // single-threaded por construção; se dois setters publicassem em paralelo,
    // correriam a lista de handlers, os contadores e a flag de reentrância, e a
    // ordem dos eventos poderia inverter. Em vez de tornar o EventBus
    // thread-safe (custo em toda publicação), o fato é ACUMULADO numa máscara
    // sob o mesmo lock da mutação, e só a `app_loop` publica — que é quem a
    // tabela do §5 já designa como dona do despacho de UI.
    //
    // GANHO DE BRINDE: coalescing. Vinte mutações de relógio entre dois ticks
    // viram UM bit, logo UM evento, logo uma repintura — não vinte. É o que a
    // ADR-006 chama de "despachante é o dono único do coalescing", e ataca
    // diretamente o custo de banda MSPI (RESOURCE-BUDGET §1.1).
    //
    // Devolvem `true` só quando o valor MUDOU. Valor igual não muta e não marca
    // nada: dedup na origem.
    bool set_clock(models::ClockState clock);
    bool set_network(models::NetworkState s);
    bool set_wifi_setup(models::WifiSetupState setup);
    bool set_weather(models::WeatherState weather);

    // ── Drenagem: EXCLUSIVA da `app_loop` ────────────────────────────────────
    // Devolve a máscara acumulada e a ZERA, atomicamente sob o lock. A
    // `app_loop` publica cada bit no EventBus DEPOIS desta chamada — portanto
    // fora da região crítica e numa única task.
    models::EventMask take_pending_events();

    // ── Leitura granular (ADR-011) ───────────────────────────────────────────
    models::ClockState clock() const;
    models::NetworkState network() const;
    models::WifiSetupState wifi_setup() const;
    models::WeatherState weather() const;

private:
    ILock& lock_;
    models::AppState state_;   // DONO ÚNICO: escrito só pelos setters acima
    models::EventMask pending_ = 0;  // protegido pelo mesmo lock do estado
};

}  // namespace core
}  // namespace nova
