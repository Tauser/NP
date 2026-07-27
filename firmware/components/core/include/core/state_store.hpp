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

#include "core/event_bus.hpp"
#include "core/lock.hpp"
#include "models/app_state.hpp"

namespace nova {
namespace core {

class StateStore {
public:
    // `bus` pode ser nulo (testes que não observam eventos, boot antes do
    // wiring). Quando presente, o StateStore é quem publica — ver abaixo.
    StateStore(ILock& lock, EventBus* bus) : lock_(lock), bus_(bus) {}

    // ── Mutação: muta E publica ──────────────────────────────────────────────
    // O setter é o ÚNICO caminho de escrita e ele mesmo publica o fato. Deixar
    // a publicação a cargo do chamador tornava possível mutar sem avisar
    // ninguém — a tela ficaria velha e o bug seria invisível.
    //
    // ORDEM, que é o ponto delicado: a mutação acontece sob o lock; o lock é
    // LIBERADO; e só então o evento é publicado. Publicar com o lock na mão
    // faria o handler rodar dentro da região crítica e, como o handler pode ler
    // o estado, isso seria deadlock (o lock do alvo não é recursivo).
    //
    // Devolvem `true` só quando o valor MUDOU. Valor igual não muta e não
    // publica: dedup na origem, porque evento redundante vira repintura e
    // repintura custa banda MSPI (RESOURCE-BUDGET §1.1). Foi uma das causas de
    // custo do baseline anterior (`ClockChanged` a 1 Hz repintando tela inteira).
    bool set_clock(uint8_t hour, uint8_t minute, bool valid);
    bool set_network(models::NetworkState s);

    // ── Leitura granular (ADR-011) ───────────────────────────────────────────
    models::ClockState clock() const;
    models::NetworkState network() const;

private:
    ILock& lock_;
    EventBus* bus_ = nullptr;
    models::AppState state_;  // DONO ÚNICO: escrito só pelos setters acima
};

}  // namespace core
}  // namespace nova
