# NovaPanel — Decisões de Arquitetura (ADRs)

> Formato curto: contexto → decisão → consequências. Numeração própria
> deste baseline. Decisão validada em baseline anterior entra aqui
> **reafirmada com a evidência**, não por herança tácita.
>
> Mudou o desenho? ADR novo **no mesmo commit**.

---

## ADR-001 — Baseline novo sem reescrever o que estava provado

**Contexto.** Este é o terceiro começo. Os dois anteriores tinham núcleo
sólido e morreram por drift documental e por um defeito de plataforma nunca
atribuído. Reescrever tudo pela terceira vez herdaria o mesmo `sdkconfig`,
o mesmo BSP e o mesmo pipeline gráfico — e portanto o mesmo defeito, com
mais código novo por cima para confundir a investigação.

**Decisão.** Documentação nova como fonte única; firmware construído por
**port seletivo crítico** do que já estava provado (núcleo de estado,
orquestrador de rede, cache, receitas de BSP). Proibido copiar wiring,
arquivo-deus ou padrão vetado.

**Consequências.** Ganha-se o conhecimento acumulado sem carregar o passivo
estrutural. Exige disciplina: cada trecho portado passa por crítica, não
por `copy`.

## ADR-002 — `STATUS.md` como fonte única de estado

**Contexto.** A causa-raiz da degradação anterior não foi técnica: foi
documentação afirmando estados divergentes num repo operado por agentes.

**Decisão.** Somente `STATUS.md` afirma estado. Os demais documentos
descrevem alvo ou política. Fechamento de etapa atualiza o STATUS **no mesmo
commit**. Divergência é bug com prioridade de crash.

**Consequência.** Um agente que leia só o STATUS nunca age sobre premissa
falsa.

## ADR-003 — O glitch de render bloqueia o roadmap

**Contexto.** Um artefato visual sobreviveu a cinco correções e a uma
reconstrução completa. Todas as correções mudaram uma variável plausível
sem instrumentar o mecanismo.

**Decisão.** **Onda 0** existe e bloqueia todas as outras: nenhuma tela de
produto entra antes de o defeito ter **causa nomeada, mecanismo explicado e
correção confirmada** pelo método do `GLITCH-PROTOCOLO.md`. Correção sem
experimento capaz de falsificá-la é proibida.

**Consequências.** Atrasa funcionalidade e é deliberado: construir por cima
de defeito não atribuído foi exatamente o que aconteceu nas duas tentativas
anteriores.

## ADR-004 — Rede: política separada da execução, 1 HTTPS por vez

**Contexto.** Validado em campo: 3 handshakes TLS simultâneos (~130 KB cada)
esgotavam a SRAM interna e derrubavam os serviços.

**Decisão.** `RequestOrchestrator` (intervalo, rate limit, breaker, backoff
com jitter) separado do `NetworkWorker` (execução serializada, gap entre
buscas). Service de dados **não tem task própria**. Truncamento de corpo é
falha do request.

## ADR-005 — HAL obrigatória com locks semânticos

**Contexto.** No baseline anterior à HAL, o BSP e centenas de linhas de
driver vazaram para o `main`, e o invariante "lock do display = mutex do
I2C compartilhado" ficou em comentários.

**Decisão.** `IBoard` (+ `MockBoard`, `WaveshareBoard`) desde a primeira
onda. `lock_ui()` e `lock_shared_i2c()` são nomes semânticos; a coincidência
física é detalhe interno da implementação real.

**Consequências.** Teste de service sem placa; `main` volta a ser wiring.
Custo: uma indireção — aceito.

## ADR-006 — UI por registro com máscara de invalidação

**Contexto.** No baseline anterior à padronização, cada par evento→tela era
um `if/else` com flag booleana no `main` (mapa N×M), e telas repintavam
inteiras a cada evento.

**Decisão.** Telas se registram por spec; o shell itera o registro; o
despachante é o **dono único** do coalescing; view-model puro por tela.
Normativo: `UI-PATTERN.md`.

## ADR-007 — Providers atrás de interface, com fixtures obrigatórias

**Contexto.** Payload externo muda sem aviso, e era o ponto com zero teste.

**Decisão.** Interface por domínio desde o primeiro commit do domínio;
service recebe a interface por injeção no `main`. Todo provider tem
fixtures versionadas — payload real capturado **mais** variantes malformadas
e truncadas — rodando no CI.

## ADR-008 — Concorrência com propriedade declarada

**Contexto.** Sincronização por convenção produziu data race real com
comentário afirmando segurança, e fila de intenções com profundidade 4 e
descarte silencioso.

**Decisão.** A tabela de tasks do `ARCHITECTURE.md` §5 é normativa. Campo
compartilhado é atômico, protegido ou trafega por fila — **declarado no
header**. Fila ≥ 16 com overflow logado e contado.

## ADR-009 — `RESOURCE-BUDGET.md` como contrato de plataforma

**Contexto.** As mitigações físicas corretas estavam espalhadas em
comentários de vários arquivos, e por isso regrediam em silêncio.

**Decisão.** O orçamento é normativo: regra de alocação por tipo de dado,
limiar de heap **com handler real**, orçamento de escrita de flash e de
rede. Feature nova declara custo **antes** de entrar no roadmap.

## ADR-010 — Buffer de DMA respeita a linha de cache

**Contexto.** O draw buffer do LVGL vive em PSRAM e é origem de DMA, mas é
alocado com o alinhamento default do LVGL (4 bytes) enquanto a linha de L2
do P4 tem 64 bytes. O caminho de PPA do mesmo componente alinha o buffer
dele a 64. Ver `GLITCH-PROTOCOLO.md` §2.1.

**Decisão.** Todo buffer que sirva de origem ou destino de DMA tem
**endereço-base e tamanho múltiplos da linha de cache**. Vale para o draw
buffer do LVGL, para buffers de PPA e para qualquer buffer futuro entregue
a um periférico.

**Status.** Hipótese principal do glitch; a confirmação experimental é
critério de saída da Onda 0. A regra vale independentemente do resultado,
porque um buffer de DMA desalinhado é bug latente mesmo que não seja *o*
bug atual.

## ADR-011 — Só a `lvgl_task` toca objetos LVGL

**Contexto.** O baseline anterior renderizava do loop principal sob o lock
do display. Funciona como exclusão mútua, mas espalha a manipulação da
árvore de widgets por duas tasks, em cores possivelmente diferentes, com
buffer em PSRAM cacheada.

**Decisão.** Render acontece **na** `lvgl_task`. As demais tasks publicam
estado e intenção. A rotina de render lê acessores granulares e **não copia
o estado inteiro** para a pilha.

**Consequência.** Remove um desvio de contrato e reduz a janela de qualquer
problema de coerência de cache.

## ADR-012 — Gate é o que a máquina verifica

**Contexto.** Critério de saída dependente de observação humana prolongada
não é gate: é esperança. E o hardware já acumula meses sem falha, então
soak não é o teste que falta.

**Decisão.** Todo critério de saída é verificável por CI, por host-check,
por script estático ou por bancada **curta e roteirizada**. Onde só a placa
resolve, o roteiro é escrito, os números são registrados no PR e a ausência
de medição é declarada explicitamente.

## ADR-013 — Limites de tamanho como gate

**Contexto.** Arquivo-deus e função longa foram o que tornou o baseline
anterior irrevisável muito antes de ficar incorreto.

**Decisão.** Os limites do `ARCHITECTURE.md` §11 (arquivo, função,
parâmetros, aninhamento) são verificados por script no CI e falham o build.

## ADR-014 — Persistência versionada e throttlada

**Decisão.** NVS com `schema_v` e migração explícita; versão futura
desconhecida → ignora sem brickar; escrita deduplicada. Cache em LittleFS
com header magic/versão/tamanho e escrita `tmp`+`rename`; mismatch →
descarte. Mudar a forma de um blob exige **nome de domínio novo**, porque a
validação é por tamanho e não por schema.

## ADR-015 — Relógio híbrido RTC ↔ NTP

**Decisão.** O RTC com bateria é a fonte de boot: hora plausível aparece
**imediatamente, sem rede**. NTP refina quando disponível. Sem hora
plausível, a UI mostra estado não-sincronizado — **nunca inventa data**.
NTP é pré-requisito funcional de HTTPS (validação de certificado).

## ADR-016 — OTA A/B antes de qualquer unidade lacrada

**Contexto.** A combinação de partição única com criptografia de flash em
modo release e anti-rollback produziria unidade sem caminho de atualização.

**Decisão.** A tabela de partições nasce com dois slots de app e `otadata`
(evita reparticionar depois). Fluxo OTA com assinatura e **rollback
automático por health-check** é pré-requisito do provisionamento seguro.
Anti-rollback só liga junto do fluxo OTA funcionando.

## ADR-017 — Produto single-locale por escolha explícita

**Decisão.** MVP em pt-BR/Brasil, mas com strings em tabela única e
timezone/localização em configuração central. i18n completa **não** é
requisito; fica barata de adotar se o produto deixar de ser pessoal.

## ADR-018 — Onda 0 tem timebox e caminho de contenção

**Contexto.** A ADR-003 torna a atribuição do glitch bloqueante. Um gate
bloqueante sem prazo e sem alternativa trava o projeto para sempre caso a
causa esteja no BSP, no componente gerenciado ou no silício — onde pode não
haver correção nossa.

**Decisão.** Timebox de **5 experimentos do protocolo ou 2 semanas de
trabalho efetivo**. Esgotado, a onda fecha pelo caminho de contenção:
hipóteses todas registradas com seu experimento, instrumentação permanente
entregue, limitação conhecida escrita no `STATUS.md` com sintoma e
frequência, e reavaliação agendada com gatilho explícito (atualização de
BSP, `esp_lvgl_port` ou ESP-IDF).

**Consequência.** Defeito conhecido, medido e contido é estado aceitável de
produto. Defeito sem nome assombrando o roadmap não é.

## ADR-019 — Gate de render por proxy verificável, não por observação

**Contexto.** "Zero artefato visual" só se verifica com olho humano e
vídeo — exatamente o que a ADR-012 proíbe, porque não automatiza e por isso
regride em silêncio.

**Decisão.** O gate passa a ser três contadores em firmware: underruns do
DSI, ocorrências de `flush_ready` antes do "trans done", e buffers de DMA
desalinhados detectados no boot. Todos devem ficar em zero. O método por
vídeo continua sendo ferramenta de **investigação**, nunca gate.

## ADR-020 — Observabilidade com saída local, nenhuma sempre ligada

**Contexto.** Métrica visível só na tela de um painel de parede é métrica
que ninguém lê; mas o produto não tem — nem quer — backend.

**Decisão.** Duas vias locais: dump para cartão SD sob comando, e endpoint
HTTP somente-leitura na LAN, desligado por default, ligado por ação na tela
e com desligamento automático. Enquanto ativo, aparece na interface.
Nenhuma telemetria sai da casa.

## ADR-021 — Operação 24/7 é requisito, não consequência

**Contexto.** O produto fica ligado permanentemente numa parede, com PSRAM
a 200 MHz e o DSI lendo o framebuffer sem parar. Nenhum baseline anterior
tratou regime térmico ou envelhecimento de painel.

**Decisão.** Temperatura entra na instrumentação desde a primeira onda, com
degradação escalonada (reduzir brilho, suspender fetcher não crítico, modo
mínimo). Conteúdo estático prolongado exige deslocamento periódico de
pixels no modo ambiente. Detalhes no `RESOURCE-BUDGET.md` §8.

## ADR-022 — Dependência com versão exata e bump revisado

**Contexto.** O firmware puxa BSP, LVGL, ESP-Hosted e drivers de um registry
externo, historicamente declarados como `"*"`. Isso quebra reprodutibilidade
e é superfície de cadeia de suprimentos.

**Decisão.** Manifesto declara **versão exata**; `dependencies.lock` é
versionado; `managed_components/` não é commitado. Subir versão é mudança
revisada, com diff lido e gates rodados. Atualização de componente do
caminho gráfico é gatilho de reavaliação do glitch.

## ADR-023 — Não desenhar interface para dado inexistente

**Decisão.** Área de tela só existe se houver provider, service e campo de
estado aprovados. Mockup exploratório pode mostrar o produto completo;
firmware, não. Estado vazio honesto é preferível a moldura mentindo sobre
capacidade.

## ADR-024 — O "glitch" eram dois defeitos; ambos atribuídos (Onda 0)

**Contexto.** A ADR-003 tornou a atribuição do glitch bloqueante. O firmware
de diagnóstico da Onda 0 (`components/board/` + `components/diag/`) foi ao alvo
(placa v1.3, ESP-IDF v5.5.4) e rodou o `GLITCH-PROTOCOLO.md` com experimentos
de uma variável. Descobriu-se que "o glitch" era **dois** defeitos distintos.

**Decisão / achado.**

1. **Hipótese 2.1 (alinhamento do draw buffer) — DESCARTADA.** Build `align=4`
   (`base%64=4`, `DMA-UNSAFE` acusado no boot) ficou **idêntico** ao `align=64`.
   `align=64` permanece por ADR-010 (bug latente independe do glitch).

2. **Defeito nº 1 — piscada branca de tela inteira = erase de flash.** Erase
   bloqueia o barramento MSPI (flash↔PSRAM) → underrun do DSI ao ler o
   framebuffer → branco. Atribuído por A/B (`NOVA_FLASH_THRASH`: com erase
   pisca, sem erase limpa). Bate com RESOURCE-BUDGET §1.

3. **Defeito nº 2 — faixa de pixels velho/misturado = tearing de framebuffer.**
   Região que atualiza a cada frame sofre tearing no caminho PPA(sw_rotate)→
   framebuffer→DSI. Atribuído por bisseção (`NOVA_STATIC_STAMP`: estático
   limpo, atualizando listra). Não é draw buffer (`double_buffer` duplo não
   corrige) nem alinhamento. Família GLITCH §2.3.

**Consequências.** A Onda 0 fecha pela **atribuição** (não pela contenção da
ADR-018): dois defeitos nomeados, com mecanismo e reprodução falsificável, e
instrumentação permanente entregue. As **correções não foram aplicadas** — são
tarefas nomeadas da Onda A, com custo declarado:
- nº 1: agendar/isolar escrita de flash para fora do render (RESOURCE-BUDGET §4);
- nº 2: config anti-tearing do `esp_lvgl_port` (`avoid_tearing` / `num_fbs≥2` /
  `full_refresh`), decidida com o caminho de render real.

Flags de diagnóstico (`NOVA_TORTURE`, `NOVA_CLARITY`, `NOVA_FLASH_THRASH`,
`NOVA_STATIC_STAMP`, `NOVA_DOUBLE_BUFFER`, `NOVA_NO_ROTATE`) e overlays
(`sdkconfig.align4`, `sdkconfig.clarity`, `sdkconfig.fix`) ficam no repo como
ferramenta de reavaliação.

## ADR-025 — Correções: nº 1 impossível (política), nº 2 corrigido

**Contexto.** Após a ADR-024 atribuir os dois defeitos, tentou-se corrigir
ambos antes de iniciar a Onda A. O resultado foi assimétrico e precisa ficar
registrado com honestidade.

**Defeito nº 1 — sem correção possível nesta placa. DECIDIDO: mitigar por
política.** O DSI lê o framebuffer a 73,7 MB/s continuamente (produto 24/7):
**não existe janela sem render** para agendar erase. A única mitigação de
hardware seria o auto-suspend do erase, e o flash desta placa (**GD
`0xC84019`**) não o implementa — `CONFIG_SPI_FLASH_AUTO_SUSPEND=y` derruba o
boot em loop (`Suspend and resume may not supported for this flash model yet`).
**Consequência normativa:** cada escrita de flash em runtime **custa uma
piscada branca**; só se controla a frequência. O `RESOURCE-BUDGET.md` §4 deixa
de ser boa prática e passa a ser **restrição dura** — dedup de NVS, cache no
máximo 1×/30 min por domínio, nenhuma escrita disparada por toque. Toda feature
que escreva flash declara o custo em piscadas **antes** de entrar (ADR-009).

**Defeito nº 2 — CORRIGIDO com `avoid_tearing`.** Reproduzido de forma
determinística e bissetado com uma variável (mesmo label, mesma API, mesmo
comprimento: estático legível / atualizando não desenha, só rasgo).

**Correção confirmada em bancada:** `avoid_tearing=true` no `esp_lvgl_port`
(que usa os framebuffers do painel DPI como draw buffers do LVGL, **com swap**)
+ `CONFIG_BSP_LCD_DPI_BUFFER_NUMS=2` + `full_refresh`. Com isso o contador
**passa a ser desenhado normalmente enquanto conta**. Atribuição limpa: o build
imediatamente anterior era idêntico exceto por essa flag.

**Erro de método corrigido:** um teste anterior concluiu "buffer duplo não
resolve" apenas com `CONFIG_BSP_LCD_DPI_BUFFER_NUMS=2`. Isso era **inválido** —
alocar dois framebuffers não faz o driver alterná-los; só `avoid_tearing` ativa
o swap. O 2º buffer foi alocado (−1,2 MB de PSRAM, medido) e nunca usado.

**Fator agravante independente, confirmado:** reduzir a fonte do texto que muda
(48 → 16) **diminuiu** o artefato mesmo antes da correção. Isso confirma o
mecanismo de *miss de glyph* documentado no `RESOURCE-BUDGET.md` §1.2 e já
descoberto no baseline anterior: glyph bitmap vive em **flash**, cada miss
bloqueia a PSRAM e rouba banda do DSI. Portanto **subsetting de fonte e
minimizar a área invalidada continuam sendo requisito de plataforma**, não
estética (`UI-LAYOUT-SYSTEM.md` §5). O baseline anterior mitigou com **um label
por dígito** no relógio; esse padrão é reaproveitado.

**Causa-raiz única.** Os defeitos nº 1 e nº 2 são o **mesmo** fenômeno — o DSI
ficar sem dados ao ler o framebuffer — com **três vias** de starvation:
(a) erase de flash bloqueando o MSPI; (b) fetch de glyph em flash bloqueando o
MSPI; (c) framebuffer sendo escrito enquanto é lido (resolvido por swap).

**Descoberta colateral: a rotação 180° por PPA NUNCA esteve ativa.** O
`esp_lvgl_port` só executa a rotação por software quando `current_rotation > 0`
(`esp_lvgl_port_disp.c:677`), e esse valor nasce em `ROTATION_0` e só muda via
`lv_display_set_rotation()` — que este firmware nunca chamou. Os campos
`rotation.mirror_*` também só são aplicados no caminho de mudança de rotação.
Resultado: a tela sempre esteve de cabeça para baixo, o caminho do PPA era
**código morto**, e o comentário "180° via PPA" na receita era falso.

**Consequências:** (a) o custo que se temia no `avoid_tearing` — "perder a
rotação por PPA" — **não existe**, não havia rotação a perder; (b) a rotação
passa a ser feita explicitamente por **hardware** (`esp_lcd_panel_mirror`) no
`init_display()`, o que é **compatível** com `avoid_tearing`; (c) `sw_rotate`
fica desligado na receita.

**CUSTO REAL da correção do nº 2:** ver ADR-026 — a solução final trocou o
backend de display e o custo mudou.

## ADR-026 — Backend de display: `esp_lvgl_adapter` no lugar do `esp_lvgl_port`

**Contexto.** A correção do defeito nº 2 (ADR-025) exigia `avoid_tearing`, mas o
`esp_lvgl_port` **não combina `sw_rotate` com `full_refresh`** — ou seja, na
prática obrigava a escolher entre **render sem tearing** e **rotação de 180°**.
O painel é montado invertido (a serigrafia define a montagem), então abrir mão da
rotação não era opção. Esgotaram-se as alternativas, todas testadas e falhas:

| Tentativa | Resultado |
|---|---|
| Espelhamento por hardware (MADCTL 0x36) | EK79007 aceita o comando e **ignora** |
| `esp_lcd_panel_swap_xy` | `not supported by this panel` |
| Rotação por PPA (`sw_rotate`) | Funciona, mas **desfaz** o `avoid_tearing` |
| Rotação por matriz do LVGL 9.4 | Não rotacionou |

Sintoma idêntico está reportado em `espressif/esp-bsp#172` (Waveshare 7": o
touch rotaciona, o display não). Em painéis DPI/vídeo o controlador só faz
*streaming* do framebuffer, o que explica o MADCTL ser aceito sem efeito.

**Decisão.** Trocar **apenas o backend de display** por
**`espressif/esp_lvgl_adapter ==0.5.3`**, que faz rotação **e** prevenção de
tearing no **mesmo pipeline** (PPA + troca de framebuffer no momento seguro). O
BSP Waveshare continua criando o painel DSI (`bsp_display_new_with_handles`).
Confirmado em bancada (2026-07-26): **rotação 180° correta e texto que atualiza
sem corrupção, simultaneamente**.

**Configuração validada:** modo **`TRIPLE_PARTIAL`** (que é o
`..._DEFAULT_MIPI_DSI` do próprio adapter) + `CONFIG_BSP_LCD_DPI_BUFFER_NUMS=3`.

**Armadilha documentada — `DOUBLE_DIRECT` + rotação NÃO funciona na 0.5.3.**
O componente é internamente inconsistente:
`esp_lv_adapter_get_required_frame_buffer_count()` devolve **2** para rotação
180° (`display_manager.c:1423`, que só considera 90/270), o fetch grava
`frame_buffer_count=2`, e então `display_manager_use_panel_buffers()` exige
**>2** para qualquer rotação ≠ 0 (`display_manager.c:1017`). O setup falha em
silêncio, o adapter cai em buffer parcial de 50 linhas e o LVGL estoura
`"DIRECT mode requires screen sized buffer(s)"` — cujo assert handler entra em
**loop infinito**, aparecendo como *task watchdog* e não como erro. Aumentar o
Kconfig **não** resolve, porque a contagem é interna ao adapter.

**Consequências.** `esp_lvgl_port` permanece no build (dependência **pública** do
BSP) mas **não é inicializado**. `lock_ui`/`lock_shared_i2c` passam a mapear para
`esp_lv_adapter_lock/unlock`. A troca ficou contida em **um único arquivo**
(`waveshare_board.cpp`) — a HAL da ADR-005 pagou-se aqui. **Custo:** 3
framebuffers = **3,6 MB de PSRAM** (de ~30 MB livres); custo de render a medir.

**Crédito.** A identificação do `esp_lvgl_adapter` como solução oficial foi
**pesquisa do autor do projeto**, não do agente — que vinha tentando contornar a
limitação do `esp_lvgl_port` em vez de substituí-lo. A consulta ao baseline v1
(que revelou o mecanismo de *glyph miss*) também partiu dele.

## ADR-027 — Fundação de setup Wi-Fi sem superfície de escuta

**Decisão:** o `SetupService` é o dono do ciclo `Unconfigured → Associating →
Connected|Failed`. A credencial é um valor transitório de `IBoard`, nunca um
campo de `AppState` ou evento. O ESP-WiFi remoto usa `WIFI_STORAGE_RAM`; a
persistência explícita é `wifi_cfg/v1` em NVS somente após 30 s com IP estável.
Falhas de associação usam backoff de 2 s até 30 s e nunca persistem senha.

**Motivo:** a P4 depende do C6/ESP-Hosted e uma reconexão não pode disputar o
render com rajadas de NVS. A disciplina de atraso, deduplicação e escrita fora
de callback de toque respeita a limitação física conhecida: cada commit pode
causar uma piscada branca (ADR-025). Separar o segredo do estado preserva o
contrato de UI e evita vazamento em log, evento ou render.

**Consequências:** o boot continua offline se NVS estiver indisponível ou se o
schema for futuro; não há erase automático. A HAL só começa a estação depois do
enlace P4↔C6 e publica o resultado por estado atômico; o serviço atualiza o
`StateStore`. Não há ainda canal de entrada de credencial, SoftAP, captive
portal nem endpoint HTTP: escolher um deles é decisão de superfície de ataque
posterior, não um atalho para colocar senha no firmware.

## ADR-028 — Provisionamento por USB físico, sem portal de rede

**Decisão:** a primeira entrada de credencial é o frame local `NPW1
<ssid-base64> <senha-base64>` pela USB serial. `UsbWifiProvisioner` roda em
task própria de 4 KiB/prioridade 2, valida apenas a forma do frame e envia a
credencial por mailbox de uma vaga, protegido pelo mutex do núcleo. Somente a
`app_loop`, dentro de `SetupService::tick()`, consome a caixa e decide
associar. O frame e a senha nunca são logados, publicados nem ecoados.

**Motivo:** permite a primeira associação sem abrir SoftAP, captive portal ou
porta LAN — superfícies que exigiriam autenticação e política próprias. Base64
é usado para delimitar bytes e não é tratado como proteção criptográfica; a
confidencialidade é o cabo físico e o host confiável, ambos dentro do modelo de
ameaça atual. A caixa separa a task de I/O da dona do estado, sem chamar serviço
de callback de driver.

**Consequências:** nesta Waveshare, a única USB física do P4 usa
USB Serial/JTAG como console **primário**, não saída secundária: só o console
primário entrega `stdin`, requisito para o frame local. O monitor serial deve
ficar fechado enquanto o utilitário local abre a COM. A senha pode existir
transitoriamente no host e no cabo, mas não no log, repositório, `AppState` ou
`EventBus`. A alternativa remota continua fora do escopo e requer ADR nova; a
escrita NVS permanece após 30 s com IP.

## ADR-029 — NTP antes da primeira sonda HTTPS

**Decisão:** `SntpService` inicia o SNTP somente após IP e entrega UTC à
`ClockService` dentro da `app_loop`; nenhum callback de rede escreve estado.
A sonda de bancada HTTPS só é habilitada depois que o `ClockState` aponta NTP
e é executada pelo único `NetworkWorker`, com o mesmo corpo de 48 KiB já
orçado. Em bancada, ela faz três consultas seriadas a `https://example.com/`
e então se desabilita até o próximo boot. Cada uma mede heap interno
antes/mínimo/durante/depois.

**Motivo:** certificados TLS exigem relógio plausível. Esta ordem impede uma
falha de certificado mascarar um problema de conectividade e preserva a regra
de um handshake HTTPS por vez. SNTP é tráfego UDP de infraestrutura, sem heap
TLS e sem concorrência com o worker HTTPS.

**Consequências:** sem Wi-Fi ou SNTP, o painel continua offline e a sonda não
abre socket. Falhas HTTPS entram no breaker do orquestrador; o resultado não
carrega payload para estado, tela ou log.

## ADR-030 — Primeiro dado real: clima de Brasília/DF via Open-Meteo

**Decisão:** o primeiro provider de produto é `OpenMeteoWeatherProvider`, com
coordenadas explícitas de Brasília/DF (`-15.793889,-47.882778`). Ele consulta
apenas a condição atual (temperatura, sensação, código, dia/noite e vento),
faz parse defensivo com fixtures real, malformada e truncada, e é injetado no
`WeatherService` pela interface `IWeatherProvider`. O service só habilita o
request após Wi-Fi+NTP, agenda-o no `NetworkWorker` único a cada 30 min e
guarda no `StateStore` um `WeatherState` de 24 B. Cache deliberadamente não
entra nesta decisão.

**Motivo:** Brasília/DF foi a localização escolhida para o MVP. Um payload
pequeno reduz consumo de corpo/parsing; coordenada evita geocoding, nova API e
ambiguidade de cidade. Separar provider de agenda e estado preserva o limite
de uma conexão HTTPS, permite fixture no host e deixa a futura preferência de
localização trocar somente a configuração/adaptador.

**Consequências:** no primeiro boot offline ainda não há dado de clima — a UI
futura deve representar ausência; após uma leitura live, falha posterior marca
o último valor como `stale`. Cache persistente/versionado, escolha de cidade e
tela ficam para suas ondas próprias; qualquer um exige nova decisão e custo de
flash/UI declarado.

## ADR-031 — Cache offline de clima em LittleFS, com throttle persistente

**Decisão:** o clima usa um blob LittleFS de 28 B (`magic`, versão, tamanho,
CRC e payload), gravado por `tmp + fsync + rename`. O cache nunca é formatado
automaticamente quando houver conteúdo/corrupção: blob ausente, truncado,
inválido ou de versão desconhecida é ignorado e o painel continua degradado.
Uma partição comprovadamente virgem (setor raiz todo `0xFF`) recebe a estrutura
inicial LittleFS uma única vez. A leitura no boot publica
`WeatherSource::kCache` com `stale=true`; leitura ao vivo posterior a substitui.
O timestamp UTC do último save viaja no blob e impede escrita em intervalo
menor que 30 min inclusive depois de reboot.

**Motivo:** a partição flash pode provocar uma piscada durante erase. Persistir
apenas após resposta live, no `app_loop` — nunca na `net_worker`, render ou
toque — conserva uma conexão HTTPS por vez e aplica o orçamento físico sem
perder utilidade offline.

**Consequências:** cache não é prova de atualidade e a UI deverá mostrar sua
origem quando a tela entrar. Falha de montagem/escrita não bloqueia o boot nem
a leitura live. A atomicidade e o codec são cobertos no host; a primeira
validação de boot sem rede é registrada no `STATUS.md`; a possível piscada de
escrita segue como medição de bancada.
