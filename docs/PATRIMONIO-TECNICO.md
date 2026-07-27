# NovaPanel — Patrimônio técnico

> **Inventário do que já custou caro para descobrir.** Cada item aqui foi
> pago com depuração real em bancada nos baselines anteriores. Nenhum deles
> se re-descobre: se algo neste arquivo for contrariado por código novo,
> o código novo está errado até prova medida em contrário.
>
> Este arquivo não afirma estado do projeto — ver `STATUS.md`.

Marcação: **[campo]** = observado em hardware; **[código]** = verificável
lendo fonte; **[fatal]** = já derrubou o produto.

---

## 1. Configuração de plataforma que é pré-requisito

| Item | Valor | Motivo |
|---|---|---|
| Revisão do silício | `CONFIG_ESP32P4_SELECTS_REV_LESS_V3=y` + `CONFIG_ESP32P4_REV_MIN_100=y` | **[campo]** o P4 rev v1.3 é rejeitado pelo bootloader sem isso |
| PSRAM | `CONFIG_SPIRAM=y`, modo HEX, 200 MHz | 32 MB no package; é onde vivem framebuffer e draw buffer |
| Cor LVGL | `CONFIG_LV_COLOR_DEPTH_16` (RGB565) | formato nativo do painel; qualquer outro custa conversão por pixel |
| Render LVGL | **parcial, nunca FULL** | **[campo]** modo FULL satura a banda de PSRAM disputada pelo DSI |
| Rotação 180° | por software/PPA (`sw_rotate=true`) | **[campo]** o EK79007 **não** gira por MADCTL: o mirror de hardware não tem efeito visível |
| `avoid_tearing` | **incompatível** com `sw_rotate` neste BSP | **[campo]** |
| Wi-Fi | `CONFIG_ESP_HOST_WIFI` **desligado** quando se usa `esp_wifi_remote` | **[campo]** ligar os dois produz `net80211: OS adapter function version error`; o Kconfig declara os modos como mutuamente exclusivos |
| mbedTLS | `CONFIG_MBEDTLS_DYNAMIC_BUFFER=y` | reduz o pico de ~130 KB por handshake |

### 1.1 O alvo precisa existir ANTES de resolver dependências

**[código]** O BSP da Waveshare declara `targets: [esp32p4]` — ele não
existe para nenhum outro alvo. Sem `sdkconfig`, o ESP-IDF assume `esp32`,
e o resolvedor de dependências falha com uma mensagem que aponta para o
lugar errado:

```text
ERROR: Version solving failed:
  - no versions of waveshare/esp32_p4_wifi6_touch_lcd_7b match ==1.0.4
```

Parece erro de versão. **Não é**: nenhuma versão daquele componente suporta
`esp32`. O sinal que denuncia está algumas linhas acima, no mesmo log:

```text
-- Building ESP-IDF components for target esp32
-- Found assembler: .../xtensa-esp32-elf-gcc.exe
```

Agrava-se com um segundo efeito: `idf.py set-target` depende de
`fullclean`, e o `fullclean` **se recusa** a apagar um `build/` que não
tenha `CMakeCache.txt` — que é exatamente o estado deixado por um configure
abortado. O resultado é um ciclo travado:

```text
set-target → fullclean → "não é um diretório de build CMake" → exit 2
```

**Como sair:** apagar `firmware/build/` na mão e rodar
`idf.py set-target esp32p4` com o diretório limpo. Uma vez que o
`sdkconfig` exista com o alvo certo, o problema não volta.

**Regra derivada:** em projeto de alvo único, o primeiro comando depois de
clonar é `idf.py set-target esp32p4` — antes de qualquer build, e antes de
deixar a extensão do VSCode tentar configurar sozinha.

## 2. Display e caminho de render

- **Backlight fica apagado até o primeiro frame.** **[campo]** Ligar o
  backlight junto com o display mostra uma tela branca antes do primeiro
  render. A ordem correta é: sobe o display → renderiza o primeiro frame →
  liga o backlight.
- **O "flicker" histórico era o backlight (GPIO32), não tearing.** **[campo]**
  Antes de investigar o pipeline gráfico, descarte o backlight.
- **Com draw buffer em PSRAM + `sw_rotate`, usar `double_buffer=false`.**
  **[campo]** Com buffer duplo, o LVGL pode reutilizar o segundo buffer
  enquanto o flush/PPA ainda o lê. Mitigação válida, **mas não é a causa
  suficiente do glitch** — o sintoma foi reproduzido mesmo com buffer único.
- **A `lvgl_task` do BSP roda em prioridade 4** **[código]**
  (`ESP_LVGL_PORT_INIT_CONFIG()`), sem afinidade de core, com timer de 5 ms.
  Qualquer task que possa preemptá-la precisa ficar abaixo disso.
- **Buffer parcial de 60 linhas** = `1024 × 60 × 2 B` = 122.880 B em PSRAM.
  Cada flush move ~245.760 B (leitura do draw buffer + escrita no
  framebuffer), sobre um tráfego contínuo de **73,7 MB/s** do DSI lendo o
  framebuffer a 60 Hz.

## 3. Rede

- **1 HTTPS por vez em todo o firmware.** **[campo]** Três handshakes TLS
  simultâneos (~130 KB cada) derrubaram a SRAM interna para ~173 KB livres e
  produziram falha em cascata. A serialização é estrutural (mutex no cliente
  HTTP), não apenas de política.
- **Corpo HTTP em SRAM interna, com teto.** Resposta maior que o teto é
  **falha do request** (conta no breaker), nunca truncamento silencioso.
- **Parser JSON alocando em SRAM interna.** Árvore de parse em PSRAM
  compete com o DSI durante o render.
- **Gap mínimo entre buscas** e escalonamento no boot: fetchers nunca sobem
  simultâneos.
- **O P4 não tem rádio.** Toda rede passa pelo link ESP-Hosted/SDIO com o
  C6. Falha ou saturação desse link é falha de rede do produto.
- **Não chamar `esp_hosted_get_coprocessor_fwversion()` no caminho do
  produto.** **[campo]** Consulta diagnóstica que gera erro ruidoso quando o
  link ainda não subiu. O bring-up correto passa por `esp_wifi_remote`.
- **Subir o transporte de forma assíncrona.** **[campo]** O link com o C6
  pode bloquear ~21 s antes de falhar; no caminho síncrono isso deixa o
  touch mudo durante o boot.
- **NTP é pré-requisito de HTTPS** (validação de certificado). Sem hora
  plausível, o TLS falha por data inválida.

## 4. Persistência e flash

- **Escrita de flash compete com o refresh do DSI.** **[campo]** Cada
  `nvs_commit`/escrita LittleFS envolve erase, e erase de flash bloqueia o
  barramento compartilhado com a PSRAM. Consequências que viraram regra:
  - persistir cache no máximo 1×/30 min por domínio;
  - dedup obrigatório em NVS (não regravar valor idêntico);
  - **nenhuma escrita de flash iniciada por callback de toque.**
- **Blobs de cache versionados** com header magic/versão/tamanho, escrita
  `tmp` + `rename` atômica, mismatch → descarte silencioso.
- **`std::string` não sobrevive a round-trip de blob.** Serializar para
  campo de tamanho fixo antes de gravar.
- **Schema de NVS versionado com migração.** Versão futura desconhecida →
  ignora o persistido, **nunca bricka**.
- **Trocar a forma de um blob exige nome de domínio novo** (ex.:
  `weather` → `weather_v2`), porque a validação é por tamanho, não por
  schema.

## 5. Proibições fatais [fatal]

| O quê | Sintoma observado |
|---|---|
| `static std::function` global | Estouro de `__cxa_atexit` → **congela no boot** |
| `bsp_audio_init(nullptr)` | Crash + coredump corrompido + **boot loop** |
| `abort()` em falha de display | **Boot loop quente**; o correto é retry com backoff + breadcrumb persistido |
| Campo novo em `AppState`/view-model sem recalcular a pilha | **Stack protection fault** na task que renderiza |
| Sombra (`shadow_width`) em widget que atualiza | Recalculada a cada repintura **e** infla o retângulo sujo pelo spread |
| `transform_*` em widget que atualiza | Força *draw layer*: alocação + segundo blit por repintura |

### 5.1 O caso da pilha da task de render

**[campo]** A rotina de render copiava o `AppState` inteiro **mais** o
view-model da tela ativa para a pilha da task que a chamava. Com a pilha
default (3.584 B), adicionar poucos campos ao estado bastou para produzir
*Stack protection fault* e boot loop.

Duas consequências permanentes:

1. **Campo novo em `AppState` ou em view-model custa pilha**, não só RAM.
   Antes de crescer qualquer um dos dois, refazer a conta.
2. A correção de fundo é **não copiar o estado inteiro para renderizar**.
   Aumentar a pilha é o remendo que segura enquanto isso; o alvo é acessor
   granular por domínio.

## 6. Concorrência

- **Barramento I2C único**: GT911 (touch) e ES8311 (codec) compartilham o
  mesmo I2C, e o polling de touch roda dentro do lock do display.
  Consequência: mexer em registrador do codec exige o mesmo lock. O
  invariante mora em **um** lugar (a HAL), com nome semântico
  (`lock_shared_i2c`), nunca em comentários espalhados.
- **Prioridade do worker de rede abaixo da `lvgl_task`.** Trabalho CPU-bound
  de TLS/parse não pode preemptar o render.
- **Fila de intenções da UI com profundidade ≥ 16 e overflow contado.**
  **[campo]** Uma fila de 4 com descarte silencioso escondeu perda de toque.
- **Coalescing de eventos de UI tem um dono só.** Re-coalescer no `main`
  produziu repintura duplicada.
- **Handler de evento roda na task de quem publicou.** Não pode tocar LVGL
  nem bloquear; se precisar, posta na fila.

## 7. Custo de fontes e assets

- **[campo]** O catálogo de fontes sem subset chegou a **1.061.735 B** no
  binário — 43 % de uma imagem de 2,4 MB. Flash sobra (8 MB por slot); o
  problema não é espaço.
- O problema real é **locality**: glifo não vive em RAM, vive em flash e
  chega por cache. Cada *miss* é uma leitura de flash que **bloqueia o
  acesso à PSRAM** — ou seja, rouba banda do refresh do DSI. Um conjunto de
  11 glifos contíguos produz ordens de magnitude menos miss do que os mesmos
  11 espalhados num blob de 341 KB.
- Consequências que viraram regra de UI:
  - subsetar fonte por papel (dígitos para relógio, Latin pt-BR para texto);
  - relógio com **um label por dígito** (um tique típico troca 1 dígito, não 5);
  - ilustração como array RGB565A8 bruto desenhado direto do flash — sem
    decoder, sem buffer de decode, **0 B de RAM**;
  - PNG/JPEG proibidos em runtime.

## 8. Evidência de bancada acumulada sobre o glitch

Registro do que foi observado, para não repetir hipótese já descartada.

| Data | Observação | O que descarta |
|---|---|---|
| — | Piscada acompanha o ciclo de fetch de 3 min | — |
| — | Persiste com mbedTLS alocando em SRAM interna | TLS em PSRAM como causa isolada |
| — | Persiste com `SPIRAM_MALLOC_ALWAYSINTERNAL` | corpo HTTP/JSON em PSRAM como causa isolada |
| — | Ocorre também na rajada inicial de boot (3 fetches em ~5 s) | contenção de barramento como causa isolada |
| — | Worker de rede confirmado abaixo da `lvgl_task` | inversão de prioridade |
| — | Persiste com `double_buffer=false` | sobreposição de draw buffer como causa suficiente |
| — | Um glitch coincidiu com `slow render = 101.507 µs` e **13 flushes** por evento de relógio, **sem fetch em andamento** | causa puramente de rede |
| — | Heap interno caiu de ~353 KB para ~284 KB durante fetch; PSRAM estável | pressão de PSRAM |
| — | Um glitch ocorreu ~3 s **depois** da atualização da hora | correlação estrita com o instante do `ClockChanged` |
| — | Espera de flush/PPA medida em ~5 ms (o ciclo lento eram ~100 ms de render) | flush lento como gargalo |
| 2026-07-26 | Draw buffer lido no boot: `base=0x4812CC80`, `%64=0` no build de produção (`align=64`) | — (o build default alinha por construção; não testa 2.1) |
| 2026-07-26 | `align=4` forçado (`base%64=4`, `DMA-UNSAFE`): tela **idêntica** ao `align=64`, ~9 min de flush máximo | **hipótese 2.1 (alinhamento do draw buffer)** |
| 2026-07-26 | Torture puro (só render): reproduz o **tearing de região** (defeito nº 2), **não** a piscada branca | piscada branca como defeito de render puro |
| 2026-07-26 | `NOVA_FLASH_THRASH` (erase de flash a cada 500 ms, ~12,7 ms cada): **piscada branca de tela inteira** sincronizada; A/B com o mesmo build sem erase = tela limpa | **ATRIBUI o defeito nº 1** = underrun do DSI por contenção de MSPI no erase de flash |
| 2026-07-26 | `NOVA_STATIC_STAMP`: label **estático** limpo; **atualizado a cada frame** mostra faixa de pixels velho/misturado (cores de outra região) | **ATRIBUI o defeito nº 2** = tearing de framebuffer em região atualizada (família §2.3) |
| 2026-07-26 | `NOVA_DOUBLE_BUFFER=1` (draw buffer duplo): tearing **persiste** | reuso de *draw buffer* único como causa do nº 2 (é framebuffer/DSI, não draw buffer) |
| 2026-07-26 | Flash da placa lido no boot: **`chip_id=0xC84019`** (GD, 32 MB) — **não** está na lista de chips com suspend do ESP-IDF | mitigação de hardware para o defeito nº 1 |
| 2026-07-26 | `CONFIG_SPI_FLASH_AUTO_SUSPEND=y` → **BOOT LOOP**: `E spi_flash: Suspend and resume may not supported for this flash model yet` + `assert failed: __esp_system_init_fn_init_flash`. O IDF recusa em **runtime** (o Kconfig aceita) | qualquer correção do nº 1 por auto-suspend — **NÃO REABILITAR** |
| 2026-07-26 | `CONFIG_BSP_LCD_DPI_BUFFER_NUMS=2`: 2º framebuffer alocado, confirmado por PSRAM livre 31.324 → 30.124 KB (**−1,2 MB**, o custo previsto). Tearing **permaneceu idêntico** | **framebuffer único** como causa do nº 2 |
| 2026-07-26 | `NOVA_NO_ROTATE` (sw_rotate=false, PPA fora do caminho de flush): tearing **permaneceu idêntico** | **PPA / rotação por software** como causa do nº 2 |
| 2026-07-26 | Carimbo com texto de **comprimento fixo** via `snprintf`+`lv_label_set_text` (mesma API do caso estático limpo): tearing **permaneceu**. Corrige um confound do teste anterior, que trocava `set_text`↔`set_text_fmt` junto com estático↔atualizando | formatação/largura variável do label como causa; **a bisseção estático-vs-atualizando fica válida com uma variável só** |
| 2026-07-26 | Fonte do texto que muda **48 → 16** (`NOVA_SMALL_STAMP`): artefato **diminuiu** (mas o texto ainda não desenhava) | — **confirma** o *miss de glyph* lendo flash (§1.2) como fator real e independente |
| 2026-07-26 | **`avoid_tearing=true` + `num_fbs=2` + `full_refresh`** (`NOVA_AVOID_TEARING`): contador exibe **`F001389` nítido e legível** contando (confirmado em foto). `draw_buf` passa de 122.880 B para **1.228.800 B** = framebuffer inteiro com swap | — **CORRIGE o defeito nº 2** |
| 2026-07-26 | Com `avoid_tearing`, `lv_display_set_rotation(180)` aplicado sob lock **não** rotaciona: `esp_lcd_panel_swap_xy: not supported by this panel` | rotação por software E por hardware neste painel — **a orientação tem de ser resolvida no layout ou na montagem** |
| 2026-07-26 | `lv_display_set_rotation()` chamado da task `main` **sem o lock** trava em `lv_inv_area` → task watchdog | — (regressão introduzida e corrigida; reforça a ADR-011: fora da `lvgl_task`, só sob `lock_ui`) |
| 2026-07-26 | Rotação por matriz do LVGL 9.4 (`lv_display_set_matrix_rotation` + `LV_DRAW_TRANSFORM_USE_MATRIX`) | rotação por matriz como saída — **não rotacionou** |
| 2026-07-26 | **Troca do backend para `esp_lvgl_adapter 0.5.3`, modo `TRIPLE_PARTIAL` + `num_fbs=3`: rotação 180° correta E texto atualizando sem corrupção, ao mesmo tempo** | — **RESOLVE** o impasse rotação-vs-tearing do `esp_lvgl_port` (ADR-026) |
| 2026-07-26 | `DOUBLE_DIRECT` + rotação no adapter 0.5.3 → assert do LVGL `"DIRECT mode requires screen sized buffer(s)"` → **task watchdog** (o assert handler do LVGL entra em loop infinito) | combinação `DOUBLE_*` + rotação nesta versão — **usar `TRIPLE_PARTIAL`** |

**Leitura honesta (atualizada 2026-07-26):** o que se chamava de "o glitch"
eram **dois** defeitos com causas distintas, agora **atribuídos** por
experimento de uma variável:

- **Nº 1 — piscada branca:** erase de flash bloqueia o barramento MSPI
  (compartilhado com a PSRAM) → o DSI não lê o framebuffer → **underrun** →
  branco. Intermitente em produção porque flash escreve raro (`nvs_commit`,
  cache). **SEM CORREÇÃO POSSÍVEL nesta placa** — ver abaixo.

  **Por que não tem conserto:** (a) o DSI lê o framebuffer a 73,7 MB/s
  **continuamente**, 24/7 — num painel de parede **não existe "janela sem
  render"** onde apagar flash com segurança, então "agendar fora do render" é
  conselho vazio aqui; (b) a única mitigação de hardware seria o *auto-suspend*
  do erase, e o chip desta placa (**GD `0xC84019`**) não o implementa — o
  ESP-IDF recusa em runtime e o boot entra em loop.

  **Logo: cada escrita de flash em runtime custa uma piscada branca.** O que se
  controla é a **frequência**. Isso promove o `RESOURCE-BUDGET.md` §4 de boa
  prática a **restrição dura**: dedup de NVS, cache no máximo 1×/30 min por
  domínio, nenhuma escrita disparada por toque. Limitação declarada (ADR-024).
- **Nº 2 — faixa de pixels velho/misturado numa região que ATUALIZA:**
  reproduzido de forma determinística e bissetado com **uma variável**
  (mesmo label, mesma API, mesmo comprimento de texto: **estático = limpo,
  atualizando = corrompido**). **Sem correção encontrada.**

  **Tudo isto foi testado e NÃO alterou o artefato:** alinhamento do draw
  buffer (4 vs 64), draw buffer duplo do LVGL, framebuffers do painel DPI
  (1 vs 2), PPA/`sw_rotate` ligado vs desligado, e API/largura do texto. Ou
  seja, o pipeline gráfico configurável foi **esgotado**.

  **Detector, não região especial:** cor chapada e texto estático **não podem**
  revelar o defeito (todo pixel igual / nada em trânsito). O contador é a única
  coisa na tela com detalhe fino que muda — por isso é o único lugar onde se vê.
  Presume-se que afete qualquer região atualizada.

  **CORRIGIDO (2026-07-26)** com `avoid_tearing=true` + `num_fbs=2` +
  `full_refresh`: o texto passa a ser desenhado normalmente enquanto muda.
  Atribuição limpa (o build anterior era idêntico exceto pela flag). O teste
  anterior com `num_fbs=2` **sozinho** era inválido: dois framebuffers só são
  alternados quando `avoid_tearing` está ligado. **Custo:** perde-se a rotação
  180° por PPA (`avoid_tearing` não suporta `sw_rotate`) — tem de vir da
  montagem física ou de espelhamento por hardware — mais 1,2 MB de PSRAM e o
  render de tela inteira do `full_refresh`. Ver ADR-025.

  **Fator agravante independente:** reduzir a fonte (48 → 16) já **diminuía** o
  artefato antes da correção — confirmando o *miss de glyph* lendo flash
  (§1.2). Subsetting de fonte e minimizar área invalidada continuam sendo
  requisito de plataforma.

As cinco tentativas históricas falharam porque nunca isolaram **nenhum** dos
dois com um experimento capaz de falsificá-lo. O método que atribuiu está em
[`GLITCH-PROTOCOLO.md`](GLITCH-PROTOCOLO.md); a instrumentação permanente
(`components/diag/`) fica no firmware.

## 9. Fatos de hardware que não se re-medem

Consolidados em [`HARDWARE.md`](HARDWARE.md). Os que mais afetam decisão de
firmware:

- Flash NOR 32 MB + PSRAM 32 MB no package.
- Painel EK79007, MIPI-DSI 2 lanes, 1024×600, RGB565, PHY via LDO canal 3.
- **RTC sem chip dedicado**: domínio RTC interno do P4 com bateria no pino
  `ESP_VBAT`. A bateria é **recarregável (ML1220)** — usar CR1220 comum é
  risco no circuito de carga.
- Ambas as portas USB-C vão ao P4 (mesmo MAC); nenhuma chega ao C6.
- Firmware do C6 se atualiza por Slave OTA via SDIO.
