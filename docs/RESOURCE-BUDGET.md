# NovaPanel — Orçamento de Recursos

> **Contrato físico da plataforma.** Violar este documento é regressão, não
> opção de design. Números marcados **[herdado]** foram medidos em bancada
> nos baselines anteriores; este baseline ainda não mediu nada próprio
> (`STATUS.md`).

## 1. O recurso escasso desta placa

O ESP32-P4 compartilha o barramento **MSPI** entre **flash** e **PSRAM**, e
o controlador MIPI-DSI lê o framebuffer da PSRAM **continuamente**:

```text
1024 × 600 × 2 B × 60 Hz = 73,7 MB/s  — permanente, não negociável
```

Consequência: **qualquer consumidor agressivo de PSRAM, e qualquer erase de
flash, compete com o refresh do display.** Sintomas observados **[herdado]**:

- flash branco / underrun do DSI durante download e parse de HTTP em PSRAM;
- artefato visual a cada `nvs_commit` / escrita LittleFS (erase de flash);
- SRAM interna caindo a ~173 KB com 3 handshakes TLS simultâneos
  (~130 KB cada) → falhas em cascata → breakers abrindo.

### 1.1 Custo por flush

```text
Draw buffer parcial de 60 linhas: 1024 × 60 × 2 B = 122.880 B

Por flush, com rotação 180° via PPA:
  leitura do draw buffer     122.880 B
  escrita no framebuffer     122.880 B
  ──────────────────────────────────────
  total                      245.760 B  ≈ 0,25 MB de tráfego adicional
```

Um evento que dispare 13 flushes move ~3,2 MB em rajada **por cima** dos
73,7 MB/s do DSI. É esse número que o `UI-LAYOUT-SYSTEM.md` existe para
controlar.

### 1.2 Glifos de fonte também custam banda

Glifo não vive em RAM: vive em **flash** e chega por cache. Cada *miss* é
uma leitura de flash que **bloqueia o acesso à PSRAM** — ou seja, rouba
banda do DSI. Fonte grande com charset completo = área de trabalho de
centenas de KB espalhada em flash = muitos miss.

Por isso subsetting de fonte é regra de plataforma, não otimização
cosmética (`UI-LAYOUT-SYSTEM.md` §5).

## 2. Mapa de memória e regras de alocação

| Recurso | Uso reservado | Regra |
|---|---|---|
| PSRAM (32 MB) | framebuffer, draw buffer, assets grandes | **Nada tocado em rajada durante render** (HTTP, JSON, TLS) pode viver aqui |
| SRAM interna | TLS, corpo HTTP, nós de JSON, buffers de áudio, pilhas | Tudo que é parsing ou rede aloca com `MALLOC_CAP_INTERNAL` |
| Flash (app) | ver `HARDWARE.md` | Escrita em runtime é throttlada (§4) |

Regras derivadas — **obrigatórias**:

1. **1 conexão HTTPS por vez** em todo o firmware (mutex no cliente HTTP) +
   `CONFIG_MBEDTLS_DYNAMIC_BUFFER=y`. **[herdado]**
2. Corpo HTTP em SRAM interna, com teto de **48 KB**. Resposta maior é
   **falha do request** (conta no breaker), nunca truncamento silencioso.
   Mudar o teto exige atualizar esta tabela e medir heap.
3. Parser JSON alocando em SRAM interna.
4. Render LVGL em modo **parcial**, nunca FULL. **[herdado]**
5. Rotação 180° por PPA (`sw_rotate=true`); com draw buffer em PSRAM usar
   `double_buffer=false`. **[herdado]** Mitigação válida, **não** suficiente
   para o glitch — ver `GLITCH-PROTOCOLO.md`.
6. **Buffer lido por DMA respeita a linha de cache.** O P4 tem linha de L2
   de **64 B**; buffer de origem de DMA em PSRAM deve ter **endereço-base e
   tamanho múltiplos de 64**. Isso inclui o draw buffer do LVGL
   (`CONFIG_LV_DRAW_BUF_ALIGN`), que por default fica bem abaixo disso.
   Ver `GLITCH-PROTOCOLO.md` §2.1 — é a principal hipótese aberta.

   **Regra não vale sem verificação.** Todo buffer entregue a um periférico
   passa por `board::assert_dma_safe(ptr, size)`, que valida base e tamanho
   contra a linha de cache. Em `dev` a falha é ruidosa e aborta; em `prod`
   loga em nível de erro e conta na métrica. Um alinhamento errado precisa
   aparecer **no boot**, não seis meses depois como artefato intermitente.
7. **Pilha por task** (o `xTaskCreate` do ESP-IDF recebe **bytes**, não
   words):

   | Task | Pilha | Nota |
   |---|---|---|
   | `lvgl_task` | **16 KB (partida p/ medir)** — ver 2.1 | agora também renderiza (ADR-011) |
   | `net_worker` | 8 KB | prioridade **abaixo** da `lvgl_task` |
   | `app_loop` | 8 KB | não renderiza mais; só wiring e tick |

   **Contrato de pilha de render:** a rotina de render **não copia
   `AppState` inteiro nem view-model volumoso para a pilha**. Copiar já
   causou *stack protection fault* em campo com a pilha default de 3.584 B
   (`PATRIMONIO-TECNICO.md` §5.1). Campo novo em `AppState` ou em view-model
   custa **pilha**, não só RAM: antes de crescer qualquer um dos dois,
   refaça a conta.

### 2.1 Pilha da `lvgl_task` depois da ADR-011

O valor de 16 KB é **herdado de quando a `lvgl_task` só fazia flush**. A
ADR-011 move o render para dentro dela, ou seja: a mesma task passa a
construir view-model e a percorrer a árvore de widgets.

**Reusar 16 KB sem refazer a conta repetiria o erro que já derrubou o
produto** — só que trocando de task. Antes de escrever a primeira tela:

1. Medir a marca d'água (`uxTaskGetStackHighWaterMark`) da `lvgl_task` com
   a tela mais pesada ativa.
2. Dimensionar com folga de **pelo menos 2×** sobre o pico medido.
3. Registrar o número **aqui**, com a data e a tela usada na medição.
4. Expor a marca d'água na instrumentação permanente — assim um view-model
   que cresça demais aparece como métrica, não como boot loop.

Enquanto não houver medição, o valor fica declarado como **não determinado**
e isso bloqueia o fechamento da Onda A.

**Estado (2026-07-26).** A `lvgl_task` do `esp_lvgl_port` foi configurada em
`WaveshareBoard::init_display()` com **16 KB**. A marca d'água
(`uxTaskGetStackHighWaterMark` da task `"taskLVGL"`) é exposta no dump periódico
de `diag/render_probe`.

**Primeira medição em placa (2026-07-26, silício v1.3):** `pilha livre = 13.540 B`
de 16.384 → **pico ≈ 2.844 B usados**. Folga atual ≈ 5,8×.

**Ressalva obrigatória:** essa medição é com o **carimbo de diagnóstico**
(um label + um objeto), NÃO com "a tela mais pesada" que o §2.1 exige — essa
tela ainda não existe. Portanto o número é um **piso**, não a sizing final. A
regra "folga ≥ 2× sobre o pico da tela mais pesada" continua pendente e ainda
**bloqueia o fechamento da Onda A**. O que fica provado é que 16 KB tem folga
larga para o caminho de render atual; quando houver view-model de produto,
refazer a conta.

## 3. Limiares de RAM interna

| Situação | Limiar | Ação |
|---|---|---|
| Operação normal | > 80 KB livres | — |
| `ResourceWarning` | < 60 KB | evento + log + métrica persistida |
| Crítico | < 40 KB | suspende fetchers não críticos até recuperar |

O amostrador roda a cada 5 s e só publica evento **na transição** de
limiar, nunca a cada amostra. Evento sem handler real é proibido.

## 4. Orçamento de escrita de flash

- Cache: no máximo **1 escrita a cada 30 min por domínio**. **[herdado]**
- NVS: dedup obrigatório — não regravar valor idêntico; agrupar commits.
- **Nenhuma escrita de flash iniciada por callback de toque.**
- OTA é o pior caso do barramento: roda com a UI em modo reduzido, nunca
  durante uso normal.

## 5. Orçamento de rede

| Domínio | Intervalo | Rate | Prioridade |
|---|---|---|---|
| Clima | 30 min | 6/min | Normal |
| BTC spot | 3 min | 6/min | Normal |
| USD/BRL | 60 min | 6/min | Normal |

Gap mínimo entre buscas consecutivas: **400 ms** **[herdado]**. No boot os
fetchers entram escalonados, **nunca simultâneos**.

## 6. Barramentos compartilhados

- **I2C único**: GT911 (touch) + ES8311 (codec). O polling de touch roda
  dentro do lock do display ⇒ acesso a registrador do codec exige o mesmo
  lock. Encapsulado **exclusivamente** em `lock_shared_i2c()` da HAL.
- **I2S** (áudio) independe do I2C — escrita de amostras não precisa do lock.
- **SDIO P4↔C6**: tráfego Wi-Fi compete pelo mesmo mundo físico; rajada de
  rede durante animação pesada é detectável. Medir antes de adicionar
  domínio de rede novo.

## 7. Orçamento de UI

| Item | Onde vive | Teto | Como medir |
|---|---|---|---|
| Fontes no binário | flash | 400 KB | soma no `.map` |
| Ilustrações | flash | 256 KB | soma no `.map` |
| Árvore de widgets por tela | SRAM interna | **a medir** | heap livre antes/depois do `build()` |
| Telas residentes simultâneas | SRAM interna | 4 | política de ciclo de vida |
| Flushes por `update()` | banda MSPI | **4** | contador no callback de flush |
| Duração de `update()` | CPU + banda | **16 ms** (p95) | instrumentação permanente |

O teto de widget por tela fica deliberadamente **em aberto**: a §8 exige
número medido na entrega, e inventar um valor aqui violaria o próprio
contrato.

## 8. Operação contínua: térmico e envelhecimento

O produto fica **ligado 24/7 numa parede**, com PSRAM a 200 MHz e o DSI
lendo o framebuffer sem parar. Isso não é o mesmo regime de uma placa de
bancada, e nenhum baseline anterior tratou disso.

| Item | Alvo | Ação |
|---|---|---|
| Temperatura do SoC em regime | ≤ 70 °C | amostrar, expor e registrar |
| Acima de 80 °C | — | reduzir brilho e suspender fetcher não crítico |
| Acima de 90 °C | — | modo mínimo: só relógio, sem rede |

Regras derivadas:

- A temperatura entra na instrumentação permanente desde a Onda A. Sem
  série histórica não há como distinguir "esquentou hoje" de "sempre foi
  assim".
- **Brilho influencia temperatura** — o backlight é a maior carga térmica.
  Modo noturno tem, portanto, benefício térmico além do visual.
- Nada de *busy wait* em loop de UI. Task ociosa precisa dormir de verdade;
  o consumo em repouso é o que define a temperatura de regime.
- **Envelhecimento do painel:** conteúdo estático por muitas horas (relógio
  na mesma posição) é risco de retenção de imagem. Mitigação obrigatória no
  modo ambiente: deslocamento periódico de alguns pixels.

## 9. Processo

Feature com custo relevante de RAM, rede ou flash **acrescenta linha na
tabela correspondente ANTES da implementação**, com estimativa; a estimativa
é substituída pelo número medido na entrega.

Divergência estimado → medido maior que 50 % obriga revisão da decisão em
ADR.
