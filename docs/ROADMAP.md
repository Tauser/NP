# NovaPanel — Roadmap

> Define **entregas** e **critérios de saída**. O andamento é marcado
> exclusivamente em `STATUS.md`. Itens já absorvidos pelo baseline aparecem
> explicitamente para não voltarem ao planejamento; a evidência e o estado
> verificável continuam sendo o `STATUS.md`.
>
> Numeração própria por **ondas** (A, B, …), deliberadamente diferente das
> "fases" dos baselines anteriores — citar numeração antiga é proibido, foi
> fonte real de confusão.

## 1. Metas de produto — o que "premium" significa em número

Estas metas valem para **todas** as ondas. Regressão em qualquer uma é
falha de gate, não item de backlog.

| Atributo | Meta | Onde se mede |
|---|---|---|
| Boot até primeiro frame | ≤ 2 s | bancada roteirizada |
| Boot até tela inicial | ≤ 6 s | idem |
| Toque até retorno visível | ≤ 100 ms | idem |
| Flushes por `update()` | ≤ 4 | instrumentação permanente |
| Duração de `update()` | ≤ 16 ms (p95) | idem |
| Underruns do DSI | **0** | contador em firmware (proxy §1.1) |
| `flush_ready` antes do "trans done" | **0 ocorrências** | contador em firmware (proxy §1.1) |
| Buffer de DMA desalinhado | **0** | assert de boot (ADR-010) |
| Heap interno livre em regime | > 80 KB | instrumentação permanente |
| Queda de rede | UI segue operável, dado marcado `stale` | fault injection |
| OTA malsucedida | volta sozinha para o slot anterior | ≥ 3 ciclos |
| Fontes no binário | ≤ 400 KB | `font_budget.sh` |
| Temperatura em regime 24/7 | ≤ 70 °C | sensor interno, exposto e registrado |

### 1.1 Por que proxy em vez de "zero artefato visual"

"Zero artefato" só se verifica com olho humano e vídeo — exatamente o tipo
de critério que a ADR-012 proíbe, porque não é automatizável e por isso
regride em silêncio.

As três linhas de proxy acima **são** verificáveis por máquina e cobrem os
mecanismos plausíveis: underrun do controlador (banda), `flush_ready`
prematuro (pipeline PPA/DSI) e buffer desalinhado (coerência de cache). O
método por vídeo do `GLITCH-PROTOCOLO.md` §3.1 continua sendo a ferramenta
de **investigação**; ele não é gate.

## 2. Princípios de execução

- Nenhuma onda fecha sem **todos** os critérios atendidos e `STATUS.md`
  atualizado no mesmo commit.
- Contrato estrutural (HAL, registro de telas, interfaces, propriedade de
  threads) entra na **primeira** onda. "Estrutura depois" nunca chegou nas
  duas tentativas anteriores.
- Port do código anterior é **seletivo e crítico**: portar módulo com
  revisão; proibido copiar wiring ou arquivo-deus.
- Otimizar antes da base existir é perda de foco — **exceto** custo de
  render, que é o defeito aberto e por isso vem primeiro.

---

## Onda 0 — Atribuir o glitch  🔒 **bloqueia todas as outras**

**Entregas.** Execução do `GLITCH-PROTOCOLO.md`: leitura do endereço-base
do draw buffer; teste de backlight travado; build "torture" só de render;
correlação por carimbo na tela + vídeo + log; instrumentação permanente de
render (flushes, duração, espera de flush, underruns).

**Critérios de saída — caminho A (atribuição)**

- [ ] Causa **nomeada**, mecanismo **explicado** e correção **confirmada**
      pelo mesmo método que produziu a evidência.
- [ ] Hipóteses descartadas registradas com o experimento que as descartou.
- [ ] Instrumentação de render permanente no firmware e exposta.
- [ ] `PATRIMONIO-TECNICO.md` §8 atualizado com o resultado.

> Não fechar por ausência de observação casual. "Não vi mais acontecer" não
> é critério (ADR-003).

### Timebox e saída — obrigatórios

Um gate bloqueante sem prazo e sem alternativa não é rigor: é armadilha. Se
a causa estiver no BSP, no componente gerenciado ou no silício, pode não
haver correção nossa — e o projeto não pode ficar parado esperando por ela.

**Timebox: 5 experimentos do protocolo ou 2 semanas de trabalho efetivo, o
que vier primeiro.** Esgotado o timebox sem atribuição, a Onda 0 fecha pelo
**caminho B**:

**Critérios de saída — caminho B (contenção declarada)**

- [ ] Todas as hipóteses da §2 do protocolo executadas e **registradas**,
      com o experimento e o resultado de cada uma.
- [ ] Instrumentação permanente entregue (isso vale mesmo sem atribuição:
      é o que permite detectar mudança de comportamento depois).
- [ ] **Limitação conhecida escrita** no `STATUS.md`: sintoma, frequência
      observada, impacto no uso e o que já foi descartado.
- [ ] Reavaliação agendada como item explícito, com gatilho definido — por
      exemplo, atualização do BSP, do `esp_lvgl_port` ou do ESP-IDF.
- [ ] Decisão consciente de seguir registrada como ADR.

Fechar pelo caminho B **não** é fracasso: é a diferença entre um defeito
conhecido, medido e contido, e um defeito que assombra o projeto sem nome —
que é exatamente a situação de hoje.

## Onda A — Fundação

**Base já absorvida no baseline (não replanejar).** Esqueleto de componentes;
`models/`; **HAL** (`IBoard`, `MockBoard`, `WaveshareBoard` mínima: display,
locks, brilho e RTC); tabela de partições A/B; gates estáticos e workflow de
CI; `main/` como wiring. O núcleo entregue contém `StateStore`, `EventBus`,
`ActionQueue` com overflow contado, `UiDispatcher`, pump de eventos e
`RequestOrchestrator` (prioridade, intervalo, gap global e breaker); `utils/`
contém `Status`, `Result<T>` (inclusive para tipos sem construtor padrão) e o
contrato `IHttpClient` com coletor de corpo limitado a 48 KiB.

**Entregas restantes.** `ServiceManager`; transporte ESP-IDF e `NetworkWorker`
serializado que executem o contrato HTTP; interfaces de provider; registro de
telas vazio e shell de UI. A Onda A continua aberta até cumprir todos os
critérios abaixo.

**Critérios de saída**

- [ ] `idf.py build` e `host_check.sh --app --tests` verdes no CI.
- [ ] Testes de host cobrindo estado, eventos, orquestrador e fila.
- [ ] `arch_check` sem violação de camada; `size_check` sem violação de
      tamanho; `main/app_main.cpp` < 300 linhas.
- [ ] Boot chega a um frame estável com backlight na ordem correta.
- [ ] Render acontece **na `lvgl_task`** (ADR-011), sem cópia do estado
      inteiro para a pilha.

## Onda B — Dados reais, cache e degradação

**Entregas.** Worker de rede serializado; providers atrás de interface com
fixtures reais e malformadas; cache versionado com escrita atômica e
throttle; políticas de rede do orçamento; breaker ativo fim a fim; relógio
híbrido RTC↔NTP; onboarding de Wi-Fi com schema de NVS versionado.

**Critérios de saída**

- [ ] Boot offline mostra cache com `stale` sinalizado.
- [ ] Todos os cenários de fault injection do `TESTING.md` §4 passando **no
      host**.
- [ ] Parsing 100 % coberto por fixtures no CI.
- [ ] Onboarding completo em unidade zerada; reboot reconecta sozinho.
- [ ] Hora correta imediata com RTC válido e sem rede.

## Onda C — Telas do produto

**Entregas.** Design system (tokens, grade, estilos compartilhados,
componentes); telas do MVP sobre os quatro arquétipos; fontes subsetadas
por papel; ciclo de vida de tela declarado.

**Critérios de saída**

- [ ] Nenhuma tela placeholder navegável.
- [ ] Checklist do `UI-PATTERN.md` §8 cumprido **por tela**.
- [ ] View-model de cada tela com teste de host (válido/stale/ausente).
- [ ] `ui_check.sh` verde; nenhuma tela chamando estilo direto.
- [ ] Orçamento de render respeitado por tela, com número medido no PR.
- [ ] Fontes ≤ 400 KB no `.map`.
- [ ] Nenhuma área desenhada sem fonte de dado real (ADR-018).

## Onda D — Robustez e observabilidade

**Entregas.** Coredump para flash com procedimento de triagem; contadores
persistidos (reboots, falhas por domínio, aberturas de breaker, overflows,
avisos de recurso); watermarks contínuos; tela de sistema; roteiro de fault
injection executado em placa.

**Critérios de saída**

- [ ] Crash proposital produz coredump recuperável e triado.
- [ ] Todo cenário de falha do roteiro passa **em placa**, com log anexado.
- [ ] Tela de sistema mostra heap, rede, motivo do reset, reboots e
      breakers.
- [ ] Nenhum evento de recurso sem handler real.

## Onda E — Segurança, OTA e release

**Entregas.** OTA local com assinatura e rollback automático por
health-check; procedimento de provisionamento ensaiado em unidade
sacrificável; runbooks finalizados.

**Critérios de saída**

- [ ] OTA aplicada **e revertida** com sucesso ≥ 3 ciclos.
- [ ] Unidade provisionada funcionando **e recebendo update posterior**.
- [ ] Anti-rollback coerente com o fluxo; nenhuma chave no repositório.
- [ ] `OPERATIONS.md` e `SECURITY.md` fechados com o que foi ensaiado.

---

## Depois do MVP

Sem compromisso de desenho; cada uma exige mini-PRD e ADRs próprios:

- **Painel pessoal**: modo noite, timer, agenda funcional, perfis.
- **Casa**: sensores locais, cenas, automação em LAN.
- **Mídia e ambiente**: álbum, inteligência ambiental.
- **Ecossistema**: integrações opcionais.

Cada item declara custo contra o `RESOURCE-BUDGET.md` **antes** de entrar.
