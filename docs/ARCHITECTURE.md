# NovaPanel — Arquitetura

> Arquitetura **alvo**. Não descreve estado — ver `STATUS.md`.
> Decisões e seus porquês: `DECISIONS.md`.

## 1. Objetivos

- Offline-first real, não aspiracional.
- Baixo acoplamento **verificável pela topologia de dependências**, não por
  disciplina de quem revisa.
- Crescimento por registro: tela, provider e domínio novos entram sem tocar
  no núcleo.
- Concorrência com **dono declarado**, nunca por convenção.
- Respeito estrutural ao `RESOURCE-BUDGET.md`.
- Unidades pequenas o suficiente para caberem na cabeça de quem lê.

## 2. Fluxo de dados — sem exceções

```text
Provider ──> Service ──> StateStore ──> EventBus ──> UiDispatcher ──> UI
   ^                                                                   │
   └──── RequestOrchestrator <── NetworkWorker         intenção da UI ──┘
              (política)          (execução)            via ActionQueue
```

| Peça | Responsabilidade única | Não pode |
|---|---|---|
| `Provider` | traduzir payload externo em modelo interno | conhecer UI ou StateStore |
| `Service` | regra de domínio, decisão de cache | tocar LVGL |
| `StateStore` | estado canônico; toda mutação passa aqui | conter lógica de domínio |
| `EventBus` | sinalizar mudança (tipo + int32) | carregar dado grande |
| `UiDispatcher` | coalescer eventos para a UI — **dono único** | ser replicado no `main` |
| `UI` | ler estado pronto, publicar intenção | request, persistir, tocar hardware |

Dado grande viaja pelo `StateStore`; o evento é só um sinal. Re-coalescer
fora do `UiDispatcher` é proibido — foi bug estrutural nos dois baselines
anteriores.

## 3. Camadas

```text
main/          wiring puro: monta o grafo e roda o loop.
               PROIBIDO: driver, regra de domínio, política de repintura.
components/
  core/        EventBus, StateStore, UiDispatcher, RequestOrchestrator,
               ServiceManager, ActionQueue
  models/      AppState e structs puros, sem IO — host-testáveis
  board/       IBoard + WaveshareBoard + MockBoard (HAL, §4)
  providers/   interfaces + adaptadores concretos por API
  services/    Clock, Setup, System, Market, Weather, NetworkWorker, ...
  cache/       CacheStore (LittleFS, blobs versionados, escrita atômica)
  ui/          shell, registro de telas, design system, view-models
  utils/       Result<T>/Status, http_client, helpers puros
```

**Direção de dependência (imposta pelo build, não por review):**

```text
ui/        → models, core, lvgl                    ✗ services, providers, board, rede
services/  → core, models, cache, interfaces       ✗ adaptador concreto de provider
providers/ → models, utils                         ✗ core, ui, board
board/     → BSP/drivers                           ✗ core, services, ui
main/      → tudo, só para construir e conectar
```

Um gate de CI lê os `REQUIRES` do build e falha se essa tabela for
violada. Camada não é convenção; é falha de build.

## 4. `board/` — HAL obrigatória

Toda a interação com hardware passa por uma interface pequena:

```cpp
class IBoard {
public:
  virtual bool init_display() = 0;               // falhar NÃO aborta (§8)
  virtual void start_network_transport_async() = 0;
  virtual bool lock_ui(uint32_t timeout_ms) = 0;
  virtual void unlock_ui() = 0;
  virtual bool lock_shared_i2c(uint32_t timeout_ms) = 0;
  virtual void set_brightness(int pct) = 0;
  virtual uint64_t rtc_unix_time_s() = 0;
  virtual IAudio* audio() = 0;                   // nullptr se indisponível
};
```

**Invariante físico encapsulado:** nesta placa, touch e codec de áudio
dividem o mesmo I2C, e o polling de touch roda dentro do lock do display.
`lock_shared_i2c()` **pode** ser implementado sobre o lock do display — mas
só a `WaveshareBoard` sabe disso. O resto do sistema conhece o nome
semântico. Invariante mora em um lugar, nunca em comentários espalhados.

`MockBoard` permite testar service e lógica de UI no host, sem placa.

## 5. Concorrência — propriedade declarada

| Task | Dona de | Pode |
|---|---|---|
| `lvgl_task` | objetos LVGL, render, touch | ler snapshot do estado |
| `app_loop` | fila de intenções, tick de services, despacho de UI | mutar estado |
| `net_worker` | execução HTTP (1 por vez) | mutar estado (thread-safe) |
| callbacks de driver/ISR | nada | setar `std::atomic` drenado no tick |

Regras:

1. Campo tocado por mais de uma task é **atômico, protegido por mutex ou
   trafega por fila** — e o **header declara o dono**. Comentário afirmando
   segurança sem mecanismo é proibido (já houve data race real assim).
2. `EventBus` é síncrono: o handler roda na task de quem publicou. Handler
   não toca LVGL e não bloqueia.
3. `ActionQueue` com profundidade ≥ 16; overflow **loga e conta** — nunca
   descarte silencioso.
4. Render nunca bloqueia por service lento. Operação longa nunca roda em
   callback de toque.
5. `net_worker` fica **abaixo** da prioridade da `lvgl_task`.

### 5.1 Quem renderiza

**Só a `lvgl_task` toca objetos LVGL.** O restante do sistema publica
intenção e estado; a `lvgl_task` acorda, lê o snapshot e desenha.

O baseline anterior desviou disso — renderizava do loop principal sob o
lock do display. Funcionava como exclusão mútua, mas espalhava a
manipulação da árvore de widgets por duas tasks em cores possivelmente
diferentes, com buffer em PSRAM cacheada. Além de furar o contrato, isso
amplia a janela de qualquer problema de coerência de cache
(`GLITCH-PROTOCOLO.md` §2.4).

**Consequência de projeto:** a função de render **não copia o estado
inteiro**. Ela lê acessores granulares por domínio. Copiar `AppState` +
view-model para a pilha da task que renderiza já causou *stack protection
fault* em campo (`PATRIMONIO-TECNICO.md` §5.1).

## 6. Estado

`AppState` é a fonte única de verdade em RAM.

- Todo domínio carrega `valid`, `stale`, `source` (`Live|Cache|Mock`) e
  `last_update_ms`. A UX distingue dado real, cache e ausência **porque o
  modelo distingue**.
- Leitura frequente usa acessor granular (`clock()`, `weather()`), não
  snapshot inteiro.
- Dado volumoso fica **fora** do snapshot, com acessor dedicado válido
  apenas durante a chamada de render.
- Crescer o estado é decisão com custo declarado: RAM **e pilha**.

## 7. Rede e resiliência

Política e execução separadas:

- **`RequestOrchestrator`** (política): por domínio — habilitação,
  prioridade, intervalo mínimo, rate limit, circuit breaker
  (`Closed→Open→HalfOpen`) com backoff exponencial e jitter.
- **`NetworkWorker`** (execução): task única, **1 HTTPS por vez** em todo o
  firmware, gap entre buscas, escalonamento natural no boot.

Contrato de UX: rede caiu → UI operável; API caiu → cache com `stale`; sem
dado nenhum → uma linha explicando. Truncamento de corpo é **falha** que
conta no breaker, nunca warning silencioso.

## 8. Boot e recuperação

Ordem: NVS → display → **primeiro frame** → backlight → rede → services →
cache → dados ao vivo.

- **Falha de display não chama `abort()`.** Retry com backoff; após N
  falhas, reboot com breadcrumb persistido e cadência degradada para evitar
  boot loop quente.
- Motivo do reset e contagem de reboots entram no estado no início.
- Watermarks de heap amostrados continuamente; cruzou limiar → evento com
  **handler real** (log + métrica + degradação), nunca evento órfão.
- Coredump para flash ligado **inclusive em produção**.

## 9. Persistência

- **NVS**: Wi-Fi, preferências, flags, breadcrumbs. Schema versionado com
  migração; versão futura desconhecida → ignora, **nunca bricka**. Escrita
  deduplicada.
- **LittleFS**: blobs binários com header magic/versão/tamanho, escrita
  `tmp`+`rename`, mismatch → descarte. Persistência throttlada.

## 10. UI

Regida por `UI-PATTERN.md` (contrato de tela) e `UI-LAYOUT-SYSTEM.md`
(composição e custo). Em uma frase:

> Tela é um registro `{id, build, update, invalidation_mask}`; **geometria é
> decidida no `build()`** e o `update()` só troca conteúdo dentro de caixas
> que já têm tamanho e posição finais.

## 11. Limites de tamanho e complexidade — **gate, não estilo**

Código curto não é preferência estética: é o que mantém revisão viável e
bug localizável. Verificado por script no CI.

| Unidade | Limite | Ação se estourar |
|---|---|---|
| `main/app_main.cpp` | 300 linhas | falha de CI |
| Arquivo de tela | 200 linhas | extrair componente |
| Arquivo em geral | 400 linhas | dividir por responsabilidade |
| Função | 40 linhas | extrair função nomeada |
| Parâmetros por função | 5 | agrupar em struct |
| Níveis de aninhamento | 3 | early return / extrair |

Regras de forma que acompanham:

- **Uma responsabilidade por unidade.** Se o nome do arquivo precisa de "e",
  são dois arquivos.
- **Sem god object.** Classe que conhece três camadas está errada.
- **Sem flag booleano de comportamento em API pública.** Dois nomes são
  melhores que um `bool do_thing(bool variant)`.
- **Comentário explica o porquê**, com número quando houver medição.
  Comentário que narra o código é ruído e deve ser apagado.
- **Erro por `Result<T>`/`Status`**, sem exceções. Falha é valor de retorno.

## 12. Observabilidade precisa de saída

Contador que só existe na tela de um painel pendurado na parede é contador
que ninguém lê. Há tensão real entre **"sem backend, por design"**
(`PRODUTO.md` §3) e **"observável"**, e ela precisa ser resolvida, não
declarada.

Decisão: **duas vias locais, nenhuma sempre ligada.**

1. **Dump para cartão SD** — sob comando explícito na tela de sistema.
   Grava métricas e contadores num arquivo texto. Zero superfície de rede;
   funciona sem Wi-Fi; é o caminho padrão de triagem.
2. **Endpoint HTTP só na LAN, ativado sob demanda** — desligado por default,
   ligado por ação na tela, com desligamento automático por tempo. Só leitura,
   sem autenticação porque não expõe nada sensível, e **nunca** roteável para
   fora da rede local.

Regras: nenhuma via fica ligada por padrão; nenhuma exporta segredo; ligar
a via 2 aparece na interface enquanto estiver ativa — o usuário sempre sabe
que o painel está servindo dado.

O que **não** entra: telemetria para fora de casa, serviço em nuvem, canal
permanente. Reduzir superfície continua sendo mais barato que defendê-la.

## 13. Extensibilidade

Sensor, automação, álbum, agenda, integração: cada um entra como **módulo
novo** (provider + service + tela registrada), declarando custo contra o
`RESOURCE-BUDGET.md` **antes** de entrar no roadmap.

Extensão que exige furar camada exige ADR primeiro. Nenhuma exceção foi
concedida até hoje sem custo posterior.

## 14. Testabilidade

Pirâmide detalhada em `TESTING.md`. O que a arquitetura garante:

- `core/`, `models/` e a lógica pura de service compilam e rodam no host.
- Provider tem parsing puro separado do IO — testável com fixture.
- `MockBoard` cobre o caminho de hardware.
- View-model é função pura `AppState → struct de apresentação`, testável
  sem LVGL.

Se algo só pode ser testado com a placa na mão, provavelmente está na
camada errada.
