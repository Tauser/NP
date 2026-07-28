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
Onda A - Fundação: esqueleto, HAL, CI, render limpo  [em andamento — HAL, render, core/, utils/, orquestrador, ServiceManager, transporte/worker HTTP, setup Wi-Fi/NTP/TLS em placa, provider de clima com fixtures e shell/registro de UI prontos; telas de produto e fechamento da onda pendentes]
Onda B - Dados reais, cache offline e degradação     [em andamento — cache offline de clima validado em placa; fault injection e demais critérios pendentes]
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
    **Escopo:** `start_network_transport_async()` agora sobe só o enlace P4↔C6
    de forma assíncrona; associação Wi-Fi e áudio seguem adiados.
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
    `UiDispatcher`, pump, `RequestOrchestrator` e `ServiceManager` puro. O
    manager recebe até oito serviços por referência, sela o registro ao
    iniciar, preserva a ordem em retry de `start()` e só entrega `tick()` aos
    já inicializados — ciclo de vida exclusivo da `app_loop`. O orquestrador entrega
    uma lease global por vez, com prioridade, intervalo, gap, backoff com
    jitter injetado e breaker `Closed→Open→HalfOpen`; o **core** não conhece
    HTTP, worker nem wiring de rede. O `StateStore` agora recebe no wiring um mutex
    FreeRTOS próprio (`CoreMutex`), separado do lock da UI.
  - `components/utils/` — `Status` e `Result<T>` puros, sem exceções nem
    alocação dinâmica; `Result<T>` aceita tipos de domínio sem construtor
    padrão inclusive no caminho de falha. `IHttpClient` recebe um
    `BoundedHttpBody` fornecido pelo worker, que exige 48 KiB e rejeita
    qualquer resposta maior com `kTooLarge`; ainda não há transporte ESP-IDF.
    Host check e testes nativos passam em 2026-07-27. `idf.py build` após as
    revisões de `core/` e `utils/` passou em 2026-07-27, confirmado pelo
    operador; não há flash ou validação de bancada dessas revisões.
  - `components/services/` — `ClockService` inicia pela hora plausível do RTC,
    mantém o minuto pelo relógio monotônico e expõe uma fronteira exclusiva da
    `app_loop` para a futura entrega de NTP; sem RTC, mantém estado explícito
    de hora indisponível e não derruba o boot. Ainda não há provider SNTP, Wi-Fi
    associado ou sincronização NTP real. As unidades puras do manifesto e todos
    os testes nativos, inclusive ClockService, passaram manualmente com g++ 15.2
    em 2026-07-27; `host_check.sh` não rodou como script neste sandbox porque o
    `bash` disponível não fornece `dirname`. O operador confirmou `idf.py build`
    sem erros e flash desta revisão em 2026-07-27; RTC funcional, Wi-Fi, NTP e
    HTTPS em bancada continuam não verificados. Com display ativo, o log confirmou
    `H_SDIO_DRV: Received INIT event` e `board.ws: enlace P4<->C6 pronto; Wi-Fi
    ainda nao associado` (~5,1 s de boot). A amostra inicial de render teve
    `flush/upd(ult/max)=1/12` e `p95=9,62 ms`; o máximo de 12 pertence ao warm-up
    (4 updates) e não valida o teto de regime. `NetworkWorker` host-testável recebe handlers
    registrados no wiring, obtém uma lease do `RequestOrchestrator` e executa
    no máximo uma chamada por vez; sua task alvo reserva 48 KiB de SRAM
    interna, usa pilha de 8 KiB/prioridade 3 e registra heap antes/depois. O
    transporte `EspHttpClient` aceita somente HTTPS e valida certificados pelo
    bundle do ESP-IDF; ainda há **zero handlers/providers registrados**, logo
    nenhum request é feito. O `WaveshareBoard` sobe o link P4↔C6 em tarefa
    assíncrona de 4 KiB só após o primeiro frame/backlight; isso não associa
    Wi-Fi nem afirma conectividade. `host_check --app --tests`, `arch_check`
    e `size_check` passaram em 2026-07-27. O primeiro build desta linha parou
    em `esp_timer.h` por dependência ausente em `services`; corrigido ao
    declarar `esp_timer` em `services` e `main`. O operador confirmou
    `idf.py build` + flash após a correção (`425ba7b`) em 2026-07-27. Enlace
    P4↔C6 foi confirmado em placa nesta revisão; heap TLS e HTTPS em bancada
    **não foram verificados**. A fundação de setup Wi-Fi foi adicionada após
    essa revisão: `SetupService` mantém credenciais fora de estado/eventos/log,
    aguarda 30 s de IP estável antes do único commit NVS e reconecta com backoff
    de 2–30 s; `WaveshareBoard` passa a configurar a estação remota em RAM após
    o enlace. A entrada atual é USB física: `UsbWifiProvisioner` recebe um frame
    `NPW1` sem ecoar segredo, entrega-o por mailbox protegido à `app_loop` e o
    utilitário `tools/provision_wifi_usb.ps1` pede a senha sem mostrá-la. Não há
    SoftAP, portal nem endpoint. `host_check.sh --app --tests`, todos os testes
    nativos, `arch_check` e `idf.py build` desta revisão passaram em 2026-07-27
    (ESP-IDF v5.5.4; `novapanel.bin` 0x15cc00, 83 % livre na menor partição).
    O operador confirmou o **flash** desta revisão em 2026-07-27. Log de boot
    em placa capturado às 16:17: boot limpo após reset USB, PSRAM 32 MB,
    quatro buffers DMA-safe, primeiro frame em ~1,02 s e enlace P4↔C6 ativo
    em ~5,18 s; nenhum panic/abort/erro de driver. A mensagem inicial do GDB
    sobre `COMx` foi corrigida pelo monitor para `\\.\COM8`, não é erro do
    firmware. Associação permanece ausente como esperado, pois ainda não há
    canal de entrada de credenciais. O operador confirmou o flash da revisão de
    provisionamento USB e a mensagem `usb.provision: pronto para frame NPW1 via
    USB fisico`, mas a primeira tentativa não foi recebida: a USB única estava
    como console secundário, que só entrega saída. A configuração foi corrigida
    para USB Serial/JTAG como console primário (único que entrega `stdin`);
    `idf.py build`, `host_check --app --tests`, testes nativos e `arch_check`
    passaram em 2026-07-27 (`novapanel.bin` 0x1594a0, 83 % livre). O flash da
    correção pela COM8 concluiu com hash verificado e reset automático. Em
    bancada, a entrada USB foi recebida e a associação foi confirmada por
    `esp_netif_handlers: sta ip` e `board.ws: Wi-Fi associado e com IP` aos
    ~84 s de boot. Após aguardar o período estável e reiniciar, a placa
    reassociou sozinha e recebeu IP aos ~14 s: persistência NVS e reconexão
    automática estão confirmadas em bancada. A implementação de SNTP e da
    sonda HTTPS serializada compilou no alvo (`novapanel.bin` 0x15b020,
    83 % livre), com host check, testes nativos e arquitetura verdes. O flash
    pela COM8 concluiu com hash verificado e reset automático. Em bancada, NTP
    sincronizou e três sondas HTTPS seriadas validaram certificado, retornaram
    200 com corpo de 559 B e mantiveram o heap interno mínimo durante TLS em
    181/180/**179 KiB** (piso: 80 KiB); antes/depois: 206/204, 202/202 e
    200/202 KiB. Render permaneceu em 1 flush/update e a folga da `lvgl_task`
    em 13.292 B. A correção final que faz o log `3/3` ocorrer somente após a
    última resposta foi flashed pelo operador. O custo TLS está validado nesta
    bancada.
  - `components/providers/` — contrato UTC (`ITimeProvider`) e primeiro
    adapter real: `IWeatherProvider`/`OpenMeteoWeatherProvider` para
    Brasília/DF, com parse puro de condição atual e fixtures versionadas
    real/malformada/truncada (ADR-030). `WeatherService` habilita o fetch só
    após Wi-Fi+NTP e usa o `NetworkWorker` já serializado, a cada 30 min; sem
    cache, dado ausente permanece explícito e uma falha posterior marca a
    última leitura como `stale`. O estado adiciona `WeatherState` de 24 B e o
    provider não cria task, buffer HTTP ou escrita de flash. A primeira versão
    foi flashed e percorreu Wi-Fi+NTP+TLS em placa; a resposta real expôs um
    erro de parser (`current_units` era confundido com `current`), corrigido
    com fixture equivalente. Host check, testes nativos, arquitetura e build
    alvo da correção passaram em 2026-07-27 (`novapanel.bin` 0x15c350, 83 %
    livre). O operador confirmou em 2026-07-28 o flash e a consulta de clima
    funcionando em placa; Wi-Fi, NTP, TLS e parse do provider estão validados
    no mesmo fluxo serializado.
  - `components/cache/` — cache offline de clima: codec puro de 28 B com
    magic/versão/tamanho/CRC, armazenamento LittleFS por `tmp+fsync+rename`
    e timestamp UTC persistido que limita a escrita a uma por 30 min inclusive
    após reboot (ADR-031). O `WeatherService` lê cache no boot como
    `stale`, e só a `app_loop` persiste resposta live; host check/testes
    nativos/arquitetura e `idf.py build` passaram em 2026-07-28. A primeira
    tentativa revelou `storage` corrompida (`-84`) e não a formatou. Com
    autorização explícita, o operador apagou somente `storage`
    (`0x1020000`, 10 MB); uma revisão posterior inicializa LittleFS apenas se
    o setor raiz estiver todo `0xFF`, preservando mídia não vazia/corrompida.
    Essa revisão passou build (`novapanel.bin` 0x1665e0, 83 % livre) e foi
    flashed com hash verificado. O operador confirmou a montagem e a primeira
    escrita do cache em bancada. O operador confirmou também o reboot sem
    Wi-Fi: o mesmo dado foi carregado do cache com `stale=true`. A validação
    visual de eventual piscada durante a escrita ainda não foi medida.
  - `components/ui/` — `ScreenRegistry` puro aceita specs uma vez, recusa
    duplicata/capacidade excedida e é selado antes do Shell. O Shell registra
    invalidações no `UiDispatcher` sem tocar LVGL; um timer que roda na
    `lvgl_task` é o único caminho que cria/atualiza widgets. O catálogo está
    propositalmente vazio (ADR-023): não há tela ou dado fictício. Host
    check/testes/arquitetura/tamanho passaram em 2026-07-27; o operador
    confirmou `idf.py build` + flash da revisão de UI (`22862c2`) em
    2026-07-27. Não há validação visual de tela de produto nem medição
    adicional de render: o catálogo permanece vazio.
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

- **Rede da Onda A:** o código de link ESP-Hosted, worker e transporte HTTPS
  existe, mas a revisão ainda não foi compilada no alvo nem rodada em placa.
  Faltam nesta ordem: boot com display ativo, enlace P4↔C6, heap interno
  antes/durante/depois de handshake TLS, associação Wi-Fi provisionada, NTP e
  uma chamada HTTPS validada. Sem provider/credenciais, esta revisão não tenta
  associar Wi-Fi nem abrir socket.

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
