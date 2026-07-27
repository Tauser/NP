# NovaPanel — STATUS (fonte única de estado)

> **Este é o ÚNICO arquivo do repositório autorizado a afirmar o estado
> atual do projeto.** Todos os outros descrevem alvo, política ou história —
> nunca estado. Se qualquer outro arquivo contradisser este, este vence e o
> outro está com bug.
>
> Regra de atualização: todo fechamento de onda, milestone ou mudança de
> estrutura do repositório DEVE atualizar este arquivo **no mesmo commit**.
>
> Regra de honestidade: "não verificado" é uma resposta válida e obrigatória.
> Nunca registrar como feito algo que não foi executado nesta máquina.

## Estado geral

| Item | Estado | Desde |
|---|---|---|
| Baseline vigente | **2026-07** | 2026-07-25 |
| Onda atual | **Onda A — Fundação** (em andamento). Onda 0 fechada: glitch atribuído e corrigido | 2026-07-26 |
| Documentação | criada | 2026-07-25 |
| Scaffolding (VSCode, IDF, gates, CI) | criado; `idf.py build` executado com sucesso (ESP-IDF v5.5.4) | 2026-07-26 |
| Firmware | HAL (`board/`) + diagnóstico e instrumentação (`diag/`) + núcleo de estado/eventos/fila/dispatcher/orquestrador (`core/`, `models/`) + `utils/`; **valida em placa v1.3**, render sem tearing e rotação correta | 2026-07-27 |
| Hardware | validado; meses de operação contínua sem falha | herdado |
| Glitch de render | **ATRIBUÍDO — causa-raiz única: DSI sem dados ao ler o framebuffer.** 3 vias: (a) erase de flash no MSPI → **sem correção** (chip não faz suspend) → política; (b) fetch de glyph em flash → mitigar por fonte/área; (c) framebuffer escrito enquanto lido → **CORRIGIDO** (esp_lvgl_adapter, ADR-026), com rotação 180° preservada. 2.1 (alinhamento) descartada | 2026-07-26 |
| Build PROD validado | não | — |
| OTA operacional | não | — |

## Ondas

```text
Onda 0 - Atribuir o glitch de render                 [FECHADA — causa atribuída e corrigida]
Onda A - Fundação: esqueleto, HAL, CI, render limpo  [em andamento — HAL, render, core/, utils/, orquestrador e contrato HTTP prontos; transporte/worker/providers/UI pendentes]
Onda B - Dados reais, cache offline e degradação     [não iniciada]
Onda C - Telas do produto                            [não iniciada]
Onda D - Robustez comprovável e observabilidade      [não iniciada]
Onda E - Segurança, OTA e release                    [não iniciada]
```

Critérios de saída de cada onda: `docs/ROADMAP.md`. **Nenhuma onda conta
como entregue sem todos os critérios atendidos e este arquivo atualizado.**

## O que este repositório contém hoje

- **Documentação de baseline** (`docs/`) e referência visual (`design/`).
- **Scaffolding de projeto**: `NovaPanel.code-workspace` (duas raízes: repo
  e `firmware/`), `.vscode/` (editor + clangd + tasks),
  `firmware/.vscode/` (IDF com `adapterTargetName=esp32p4`, OpenOCD e
  debug JTAG), `.clangd`, `.editorconfig`, `.gitignore`, `.gitattributes`,
  `.github/workflows/ci.yml`.
- **Esqueleto de firmware**: `firmware/CMakeLists.txt` (agora com a opção de
  build `NOVA_TORTURE`), `partitions.csv`, `sdkconfig.defaults` (com as
  receitas de plataforma e o `CONFIG_LV_DRAW_BUF_ALIGN=64` da ADR-010),
  `idf_component.yml` com versões exatas.
- **Firmware da Onda 0** (2026-07-26):
  - `components/board/` — HAL: `IBoard` (puro), `WaveshareBoard` (BSP +
    esp_lvgl_port, receita: draw buffer de 60 linhas em PSRAM,
    `double_buffer=false`, PPA 180°) e `MockBoard`. `lock_ui`/`lock_shared_i2c`
    compartilham o mesmo lock (invariante do §6 encapsulado só aqui).
    `board::assert_dma_safe` + `dma_check` puro (ADR-010).
    **Escopo:** `start_network_transport_async()` e `audio()` do §4 estão
    ADIADOS (sem rede/áudio na Onda 0, ADR-023).
  - `components/diag/` — diagnóstico de boot (alvo/silício/PSRAM/heap;
    por draw buffer: base, tamanho, `%64`, `assert_dma_safe`, veredito 2.1);
    instrumentação permanente de render (flushes/update, duração p50/p95/máx,
    espera de flush, marca d'água da lvgl_task, heap/PSRAM); gerador de
    conteúdo com "carimbo" e modo torture. Lógica de veredito e de métricas
    é pura e host-testada (`boot_report`, `render_metrics`).
  - `main/` — `app_main` magro delega para `nova::app::run()` (`boot.cpp`):
    display → primeiro frame → **backlight só depois do primeiro frame**.
  - `firmware/tests/native/` — testes nativos (`test_all.cpp`) da lógica pura.
  - `components/core/` — `StateStore`, `EventBus`, `ActionQueue`,
    `UiDispatcher`, pump e `RequestOrchestrator` puro. O orquestrador entrega
    uma lease global por vez, com prioridade, intervalo, gap, backoff com
    jitter injetado e breaker `Closed→Open→HalfOpen`; não há HTTP, worker ou
    wiring de rede ainda. O `StateStore` agora recebe no wiring um mutex
    FreeRTOS próprio (`CoreMutex`), separado do lock da UI.
  - `components/utils/` — `Status` e `Result<T>` puros, sem exceções nem
    alocação dinâmica; `Result<T>` aceita tipos de domínio sem construtor
    padrão inclusive no caminho de falha. `IHttpClient` recebe um
    `BoundedHttpBody` fornecido pelo worker, que exige 48 KiB e rejeita
    qualquer resposta maior com `kTooLarge`; ainda não há transporte ESP-IDF.
    Host check e testes nativos passam em 2026-07-27. `idf.py build` após as
    revisões de `core/` e `utils/` passou em 2026-07-27, confirmado pelo
    operador; não há flash ou validação de bancada dessas revisões.
- **Gates**: `tools/scripts/` — `host_check`, `arch_check`, `size_check`,
  `ui_check`, `hygiene`, `check_all`.

### Verificado nesta máquina (2026-07-26, g++ 15.2, sem ESP-IDF)

- `bash firmware/tests/native/run_host_tests.sh` → **PASS** (dma_check,
  veredito 2.1, RenderMetrics, MockBoard).
- `bash tools/scripts/check_all.sh`: `arch_check` **PASS**, `size_check`
  **PASS**, `ui_check` **SKIP** (sem UI), `host_check` **PASS** (as 3
  unidades puras compilam com `-Wall -Wextra -Werror`; `app_main.cpp`
  passa em `-fsyntax-only`; testes nativos rodam).
- **`hygiene` FALHA neste sandbox** — e **só** na linha
  `dependencies.lock ... nao esta versionado`. Causa: este checkout **não é
  um repositório git** aqui, então o `git ls-files` do gate erra. É
  falso-positivo de ambiente, não defeito de código: nenhum arquivo novo
  casa com padrão proibido, e no checkout git real o `dependencies.lock` está
  versionado (o `check_all` já passou lá antes). `check_all` retorna 1 por
  causa disso.

### Verificado em bancada (2026-07-26, ESP-IDF v5.5.4, placa v1.3)

- **`idf.py build` + `flash` OK; boot limpo, sem crash** (`main_task`
  retornou normal). Todo o código-alvo compila e roda.
- Diagnóstico de boot em placa: `esp32p4 rev v1.3`, PSRAM 32 MB, heap interno
  livre 391 KB. `draw_buf[0] base=0x4812CC80 size=122880 B` — em **PSRAM**,
  `base%64=0`, `size%64=0`, **DMA-safe**.
- **Veredito 2.1 lido na placa: `AlignedByConfig`** — o buffer está alinhado
  por construção (`align=64`), então este build **não testa** a hipótese 2.1.
- Ordem de boot confirmada: 1º frame em ~1020 ms → backlight 80% (nunca antes).
- Instrumentação de render viva (idle ~1 Hz): `dur p50/p95/max = 343/1486/35304 µs`
  (max = 1º frame de tela cheia), regime `1 flush/update` (teto 4), espera de
  flush `máx 836 µs`.
- **Marca d'água `lvgl_task` = 13.540 B livres de 16.384** (pico ≈ 2.844 B) —
  ver RESOURCE-BUDGET §2.1 (é piso, medido no carimbo, não na tela mais pesada).

### Experimento da hipótese 2.1 — EXECUTADO, resultado NEGATIVO (2026-07-26)

Método: dois builds torture-clareza mudando **uma** variável (o alinhamento),
cada um rodando com flush maximizado; comparação visual do texto estático
central (sentinela) + números de render.

- **Controle** `align=64`: `base=0x4812CC80`, `base%64=0`, DMA-safe. Tela limpa,
  render impecável por ~5,5 min (804 updates), zero crash.
- **Teste** `align=4`: `base=0x4812CC44`, **`base%64=4`, `DMA-UNSAFE`** (o
  diagnóstico gritou no boot em nível ERROR, exatamente como a ADR-010/019
  exigem). Rodou ~9 min (1299 updates). Tela **visualmente idêntica** ao
  controle; sentinela **continuou limpa**; números de render iguais (`p95≈8,2 ms`).

**Conclusão:** forçar o desalinhamento que a 2.1 acusa **NÃO reproduziu** o
artefato. **A hipótese 2.1 não é sustentada por este experimento** como causa de
glitch visível. (Ressalva: o defeito é intermitente; isto é forte evidência
contra, não prova absoluta. Mas foi o teste mais severo possível — pior caso de
alinhamento + flush máximo — e veio vazio.)

**Conclusão maior (bisseção §3.2):** em NENHUM dos builds o torture puro (só
render, sem rede/NVS/flash/touch) reproduziu o glitch, em ~15 min somados de
flush maximizado. A "piscada" que se viu na 1ª rodada era o **conteúdo de teste**
(a barra vermelho/azul), não o defeito. Por §3.2, isso aponta o gatilho como
**externo ao render** — o que descarta a família de hipóteses puramente de
render (inclui a 2.1). Suspeitos vivos: backlight/PWM (2.2 — o aviso
`ledc: GPIO 32` persiste em todo boot), erase de flash (2.5 — RESOURCE-BUDGET §1
registra "artefato a cada nvs_commit"), rede/TLS (histórico).

**Nota:** `align=64` permanece a config correta independentemente do resultado
(ADR-010: buffer de DMA desalinhado é bug latente de qualquer forma; o
diagnóstico provou que o detecta no boot).

### Atribuição (2026-07-26): são DOIS defeitos distintos, ambos reproduzidos

O que se chamava de "o glitch" eram, na verdade, **dois** artefatos com causas
diferentes. Ambos foram reproduzidos por experimento de uma variável e
bissetados. (Correção da conclusão anterior: o torture puro **não** era limpo —
o defeito nº 2 sempre esteve lá, mascarado como "andaime feio".)

**Defeito nº 1 — piscada BRANCA na tela inteira → erase de flash.** Atribuído
por A/B: build de clareza com `NOVA_FLASH_THRASH` (erase de 4 KB a cada 500 ms
na partição `storage`, ~12,7 ms cada) faz a tela inteira piscar branco
sincronizado com o erase; **sem** o flash-thrash a tela fica limpa. Mecanismo:
o erase bloqueia o barramento **MSPI** (compartilhado flash↔PSRAM), o **DSI**
não lê o framebuffer → **underrun** → branco. Bate com o sintoma herdado do
RESOURCE-BUDGET §1 ("flash branco / underrun do DSI"; "artefato a cada
nvs_commit"). Intermitente em produção porque flash escreve raro.

**Defeito nº 2 — faixa de pixels com conteúdo velho/misturado numa região que
atualiza → tearing.** Atribuído por bisseção `NOVA_STATIC_STAMP`: um label
(fonte 48) **atualizado a cada frame** exibe listras móveis com pixels de
**outra** parte da tela (as cores da barra); o **mesmo label estático** fica
perfeitamente limpo, e a barra sólida (também atualizada) não mostra porque
tearing em cor chapada é invisível. Independe do alinhamento (align=4 e 64
iguais). Mecanismo consistente com **GLITCH §2.3** (`flush_ready` antes do
`trans_done`) / consequência do `double_buffer=false` + PPA: o buffer é
reusado/lido em trânsito e a região pega conteúdo velho. **Bate com a descrição
histórica da §2.1** ("faixa de pixels com conteúdo velho ou misturado") melhor
que a piscada branca. Testado `NOVA_DOUBLE_BUFFER=1` (confirmado
`board.ws: double_buffer=1` no boot): o tearing **persiste** → **não é reuso de
draw buffer**; é tearing de **framebuffer** (PPA escreve / DSI lê). Alinhamento
e draw buffer, ambos testados e descartados. Remédio = config anti-tearing do
`esp_lvgl_port` (`avoid_tearing` / `num_fbs≥2` / `full_refresh`), decisão da Onda A.

**Por que cinco correções falharam:** mexeram em rede/PSRAM/prioridade/buffer e
nunca isolaram **nem** o erase de flash (nº 1) **nem** o tearing de região
atualizada (nº 2), porque nenhuma teve um experimento capaz de falsificá-las.

### NÃO verificado / em aberto

- **Defeito nº 2 CORRIGIDO + ROTAÇÃO 180° CORRETA, simultaneamente** —
  confirmado em bancada 2026-07-26 (ADR-026). Solução: **trocar o backend de
  display** do `esp_lvgl_port` para o **`esp_lvgl_adapter ==0.5.3`**, modo
  **`TRIPLE_PARTIAL`** + `CONFIG_BSP_LCD_DPI_BUFFER_NUMS=3`. O `esp_lvgl_port`
  não combina `sw_rotate` com `full_refresh` — era essa incompatibilidade que
  obrigava a escolher entre render limpo e orientação certa.
- **Migração contida em 1 arquivo** (`waveshare_board.cpp`) — a HAL da ADR-005
  cumpriu o papel. `esp_lvgl_port` segue no build (dep. pública do BSP) mas
  **não é inicializado**.
- **NÃO usar `DOUBLE_DIRECT` com rotação** no adapter 0.5.3: inconsistência
  interna leva a assert do LVGL e *task watchdog*. Detalhe na ADR-026.
- **Configuração MIGRADA para `sdkconfig.defaults`** (`CONFIG_BSP_LCD_DPI_BUFFER_NUMS=3`)
  e o overlay temporário `sdkconfig.fix` foi removido. Flags de build obsoletos
  (`NOVA_AVOID_TEARING`, `NOVA_DOUBLE_BUFFER`, `NOVA_NO_ROTATE`) removidos: eram
  experimentos sobre o backend antigo. Restam como ferramenta de reavaliação os
  overlays `sdkconfig.align4` (retestar 2.1) e `sdkconfig.clarity` (carimbo
  legível), e os flags `NOVA_TORTURE`, `NOVA_CLARITY`, `NOVA_SMALL_STAMP`,
  `NOVA_STATIC_STAMP`, `NOVA_FLASH_THRASH`.
- **NÃO é defeito:** o fundo acinzentado com borda vermelha que aparece nas
  fotos é artefato de **câmera** (PWM do backlight + rolling shutter). Na tela
  real o fundo é preto e limpo — verificado a olho. Registrado aqui porque
  fotografar esta placa induz a erro repetidamente: barra sólida sai listrada,
  preto sai acinzentado. **Foto não é evidência de render neste hardware;**
  confirmação é sempre a olho.
- **Rotação 180°: RESOLVIDA** pelo pipeline do `esp_lvgl_adapter` (ADR-026),
  confirmada em placa. Histórico, para não se repetir: por muito tempo ela
  **nunca esteve ativa** (`lv_display_set_rotation` jamais era chamado, o
  caminho do PPA era código morto); depois se tentou espelhamento por hardware,
  que o EK79007 **aceita e ignora** (ver `docs/HARDWARE.md`).
- ✅ **RESOLVIDO — perfil normal validado em placa** (2026-07-26): build sem
  flags de diagnóstico, corrida de ~38 min / 2.304 updates. Confirmados no log:
  3 framebuffers, rotação 180°, **4 buffers verificados e todos DMA-safe**
  (3× 1.228.800 B em PSRAM + 102.400 B em SRAM interna), sem
  `ledc: GPIO 32`, heap e PSRAM estáveis do início ao fim.
- ✅ **RESOLVIDO — pilha da task de UI:** 13.164 B livres de 16.384 →
  **pico 3.220 B, folga 5,1×** (RESOURCE-BUDGET §2.1). Ressalva: é **piso**,
  medido no carimbo de diagnóstico e não na "tela mais pesada", que só existirá
  na Onda C — refazer lá.
- ✅ **RESOLVIDO — custo de render do `TRIPLE_PARTIAL`:** 1 flush/update
  (teto 4), p95 = 9,25 ms (teto 16 ms), espera de flush ~5 µs. Registrado em
  RESOURCE-BUDGET §7.1, com comparação contra o backend anterior.
- **Defeito nº 1 (erase de flash) segue sem correção** — é limite de hardware
  (ADR-025), mitigado por política no RESOURCE-BUDGET §4.
- ~~`W ledc: GPIO 32 is not usable`~~ — **explicado e corrigido**: era chamada
  duplicada de `bsp_display_brightness_init()` (o BSP já a faz dentro de
  `bsp_display_new_with_handles`). Não era evidência da hipótese 2.2, apesar de
  ter sido tratado como tal durante a investigação.
  - Coredump antigo (8228 B) na partição `coredump`, de baseline anterior —
    decodificar ou apagar.

## Patrimônio herdado (validado, não refazer)

Conhecimento e hardware vêm dos baselines anteriores e são reusados, não
reconstruídos. O inventário completo está em `docs/PATRIMONIO-TECNICO.md`.
Resumo do que é considerado **provado**:

- Placa, BSP, display MIPI-DSI, touch GT911, transporte ESP-Hosted P4↔C6.
- Receitas de plataforma: `sdkconfig` mínimo do P4 rev v1.3, rotação por
  PPA, backlight só depois do primeiro frame, 1 HTTPS por vez, corpo HTTP
  em SRAM interna, throttle de escrita em flash.
- Proibições fatais confirmadas em campo (boot freeze por `static
  std::function` global; crash por `bsp_audio_init(nullptr)`; estouro de
  pilha da task `main`).
- Padrões de arquitetura que sobreviveram a auditoria: StateStore/EventBus,
  RequestOrchestrator + worker único, registro de telas por spec,
  view-model puro por tela.

## Dívidas e riscos abertos

- ~~Glitch de render sem causa atribuída~~ — **RESOLVIDO** (ADR-024/025/026).
  Causa-raiz: o DSI fica sem dados ao ler o framebuffer. Duas das três vias
  estão corrigidas; a terceira é limite de hardware, abaixo.
- **Erase de flash provoca piscada branca — SEM CORREÇÃO POSSÍVEL.** O flash
  desta placa (GD `0xC84019`) não implementa suspend, e num painel 24/7 não
  existe janela sem render. **Cada escrita de flash em runtime custa uma
  piscada.** Vira restrição dura do `RESOURCE-BUDGET.md` §4 (dedup de NVS,
  cache no máximo 1×/30 min, nenhuma escrita disparada por toque). Toda feature
  que escreva flash declara o custo em piscadas **antes** de entrar (ADR-009).
  **Risco vivo para a Onda B**, quando cache e NVS passarem a existir.
- **Fetch de glyph em flash também rouba banda do DSI** (RESOURCE-BUDGET §1.2).
  Mitigação é disciplina de UI, não configuração: subsetting de fonte e
  minimizar a área invalidada (o baseline anterior usava um label por dígito no
  relógio). **Risco vivo para a Onda C.**
- **`esp_lvgl_adapter` fixado em 0.5.3 com bug conhecido:** modos `DOUBLE_*`
  não funcionam com rotação. Há `static_assert` impedindo a combinação, mas
  qualquer upgrade do componente exige revalidação em bancada (ADR-026).
- **Medições de render/pilha são pisos, não sizing final.** Foram feitas com o
  carimbo de diagnóstico; a "tela mais pesada" só existirá na Onda C e obriga
  a refazer a conta (RESOURCE-BUDGET §2.1).

## Histórico de fechamento de ondas

- **Onda 0 — Atribuir o glitch (2026-07-26): fechada por ATRIBUIÇÃO.** Dois
  defeitos nomeados com mecanismo e reprodução falsificável (ADR-024): (1)
  piscada branca = underrun do DSI por erase de flash no MSPI; (2) tearing de
  framebuffer em região atualizada (§2.3). Hipótese 2.1 (alinhamento) descartada
  por experimento. Instrumentação permanente entregue (`components/diag/`).
  **Correções não aplicadas** — são tarefas nomeadas da Onda A.
