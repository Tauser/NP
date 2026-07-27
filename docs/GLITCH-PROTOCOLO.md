# Protocolo de atribuição do glitch de render

> O artefato visual ("piscada") é o risco número um do produto. Ele
> sobreviveu a **cinco** correções e a uma reconstrução completa de
> baseline. Este documento existe para quebrar esse ciclo.
>
> Não afirma estado — ver `STATUS.md`.

---

## 1. Por que as cinco tentativas anteriores falharam

Nenhuma delas foi errada. Todas foram **prematuras**: mudaram uma variável
plausível e depois perguntaram "melhorou?" a um sintoma intermitente, cuja
janela de reprodução varia de segundos a minutos.

| Tentativa | O que assumia | Resultado |
|---|---|---|
| mbedTLS alocando em SRAM interna | TLS em PSRAM roubava banda | persistiu |
| `SPIRAM_MALLOC_ALWAYSINTERNAL` | corpo HTTP/JSON em PSRAM roubava banda | persistiu |
| Prioridade do worker de rede abaixo da `lvgl_task` | inversão de prioridade preemptava o render | persistiu (e a prioridade já estava correta) |
| `double_buffer=false` | flush/PPA lia o buffer sendo reescrito | persistiu |
| Filtrar `ClockChanged` para só a topbar | repintura desnecessária a 1 Hz | reduziu custo, **não eliminou o sintoma** |

O padrão: **cinco correções, zero atribuições.** Um sintoma intermitente só
se atribui com um método que ligue o instante do artefato a um evento
registrado. Enquanto isso não existir, qualquer correção nova é palpite com
custo de reflash.

**Regra desta baseline:** nenhuma correção do glitch entra sem, antes, um
experimento que possa *falsificá-la*.

---

## 2. Hipóteses ranqueadas

Ordenadas por (probabilidade × facilidade de testar). As três primeiras são
baratas e nunca foram testadas.

### 2.1 Alinhamento do draw buffer menor que a linha de cache — **[não testada]**

**A mais promissora.** Evidência em código, não em suposição:

O draw buffer do LVGL vive em **PSRAM** (`buff_spiram=true`) e é a **origem
do DMA** que alimenta o painel. Ele é alocado assim
(`esp_lvgl_port/src/lvgl9/esp_lvgl_port_disp.c`):

```c
buf1 = heap_caps_aligned_alloc(CONFIG_LV_DRAW_BUF_ALIGN,
                               buffer_size * color_bytes, buff_caps);
```

E a configuração vigente do projeto tem:

```text
CONFIG_LV_DRAW_BUF_ALIGN=4          ← alinhamento do draw buffer
CONFIG_CACHE_L2_CACHE_LINE_SIZE=64  ← linha da cache L2 do P4
```

Ou seja: um buffer lido por DMA, alinhado a **4 bytes**, numa arquitetura
cuja linha de cache tem **64 bytes**.

O que reforça a suspeita: o *mesmo componente*, no seu caminho de PPA,
alinha o buffer dele corretamente (`common/ppa/lcd_ppa.c`):

```c
ppa_ctx->buffer_size = ALIGN_UP(cfg->buffer_size, CONFIG_CACHE_L2_CACHE_LINE_SIZE);
ppa_ctx->buffer = heap_caps_aligned_calloc(CONFIG_CACHE_L2_CACHE_LINE_SIZE, ...);
```

Os autores do componente consideram alinhamento por linha de cache
necessário para buffer de DMA — mas o draw buffer do LVGL escapa dessa
regra e fica com o que o Kconfig do LVGL disser.

**Mecanismo proposto:** quando a CPU escreve no buffer pela cache e o
DMA lê da PSRAM, é preciso um *writeback*. Se a primeira ou a última linha
de cache do buffer for **compartilhada com outra alocação**, o writeback
pode não cobrir exatamente o que o DMA vai ler — ou uma linha suja vizinha
pode sobrescrever região já lida. O resultado é uma faixa de pixels com
conteúdo velho ou misturado: exatamente um "flash" intermitente, cuja
ocorrência depende de onde caiu o retângulo sujo.

Isso explica por que o defeito é **intermitente**, por que **acompanha
atividade de render** (mais flush = mais exposição) e por que **nenhuma das
cinco tentativas anteriores tocou nele** — todas mexiam em rede,
prioridade ou buffer duplo.

O tamanho não é problema: `1024 × 60 × 2 B = 122.880 B` já é múltiplo de 64.
**Só o endereço-base está em questão.**

**Teste (uma linha de `sdkconfig`):**

```text
CONFIG_LV_DRAW_BUF_ALIGN=64
```

**Como falsificar:** logar o endereço de `disp_ctx->draw_buffs[0]` no boot.
Se `addr % 64 == 0` já acontecer por sorte do alocador, esta hipótese
**não explica** o defeito e cai no ranking — sem gastar um reflash.
Faça essa leitura **antes** de mudar qualquer coisa.

### 2.2 Backlight, não pipeline gráfico — **[historicamente confirmado uma vez]**

O `PATRIMONIO-TECNICO.md` §2 registra: **o "flicker" histórico era o
backlight (GPIO32)**, não tearing. O backlight é PWM; qualquer perturbação
no duty, no clock do LEDC ou na alimentação aparece como piscada de tela
inteira — indistinguível, a olho nu, de um artefato de render.

**Teste:** fixar o backlight em nível constante (sem PWM, ou duty 100 %
travado) e observar. Se o artefato some, o problema **nunca foi gráfico**.
Custa uma linha e elimina metade do espaço de busca.

**Sinal diagnóstico:** artefato que afeta a **tela inteira e uniformemente**
aponta para backlight/alimentação. Artefato em **faixa/região** aponta para
o pipeline de render.

### 2.3 `flush_ready` sinalizado antes do DMA terminar — **[não testada]**

Com `sw_rotate`, o caminho é: LVGL desenha → PPA rotaciona → DSI transfere.
Se `lv_disp_flush_ready()` for chamado quando o **PPA** terminou mas o
**DSI** ainda está lendo, o LVGL libera o buffer para a próxima área e passa
a escrever por cima do que está sendo transferido — com `double_buffer=false`
não há segundo buffer para absorver isso.

**Teste:** instrumentar o callback de flush registrando (a) instante da
chamada, (b) instante do `flush_ready`, (c) instante do callback de "trans
done" do painel. Se `flush_ready` vier antes do "trans done", está
encontrado.

### 2.4 Duas tasks tocando LVGL — **[estrutural, provável agravante]**

A arquitetura manda que **só a `lvgl_task` toque objetos LVGL**. Na prática,
o baseline anterior renderizava a partir do loop principal, protegido pelo
lock do display. Funciona como exclusão mútua, mas significa que a árvore de
widgets é modificada por uma task e desenhada por outra, em cores possivelmente
diferentes (`task_affinity = -1`), com o buffer em PSRAM cacheada.

Isso não produz o artefato sozinho, mas **amplia a janela** de qualquer
problema de coerência de cache — e é um desvio de contrato que vale corrigir
de todo modo.

### 2.5 Erase de flash durante render — **[mitigado, não eliminado]**

Já existe throttle (1×/30 min por domínio) e dedup de NVS. Continua na lista
porque erase bloqueia o barramento compartilhado com a PSRAM. **Teste:**
build sem nenhuma escrita de flash em runtime.

### 2.6 Underrun real do DSI — **[nunca medido]**

Ninguém verificou se o controlador **relata** underrun. Se o driver expõe
contador ou callback de erro, um contador incrementando junto com o artefato
transforma "piscada" em número — e prova que é banda. Se nunca incrementa,
banda está descartada e sobram coerência de cache e backlight.

---

## 3. Método: tornar o defeito atribuível

O problema central é ligar **um instante visual** a **um evento de
firmware**. Duas técnicas resolvem isso sem soak.

### 3.1 Correlação por vídeo com carimbo na tela

1. Desenhar, num canto da tela, um contador monotônico grande e legível,
   atualizado a cada frame, junto de uma sigla do último evento processado.
2. Filmar a tela a 60 fps (celular moderno serve; 120/240 fps é melhor).
3. Quando o artefato aparecer, avançar o vídeo quadro a quadro e **ler o
   contador no quadro corrompido**.
4. Cruzar esse número com o log serial, que registra o mesmo contador a cada
   flush, com evento, duração e número de flushes.

Isso troca "aconteceu em algum momento nesses 3 minutos" por "aconteceu no
frame 41.207, durante o flush da área (0,180)-(1023,240), no evento
`MarketChanged`". A partir daí, o defeito é depurável como qualquer outro.

**Custo:** algumas dezenas de linhas e um celular. **Sem soak.**

### 3.2 Bisseção por construção — o build "torture"

Um build mínimo que faz **só render**:

- sem Wi-Fi, sem HTTP, sem NTP;
- sem NVS, sem LittleFS, sem cache;
- sem áudio, sem touch;
- uma tela que alterna conteúdo num ritmo agressivo (ex.: relógio sintético
  a 10 Hz) para maximizar flushes.

Duas saídas possíveis, ambas conclusivas:

- **Glitch aparece:** o defeito é 100 % do caminho de render/plataforma.
  Rede, flash e cache saem definitivamente do espaço de busca — o que
  invalida de uma vez as três primeiras tentativas históricas.
- **Glitch não aparece em janela equivalente:** o gatilho é externo ao
  render. Religue **um** subsistema por vez (flash → rede → touch), na
  ordem de custo de barramento, até reproduzir.

Este é o experimento de maior valor por hora investida e deve ser o
primeiro depois da leitura de endereço da §2.1.

### 3.3 Instrumentação permanente (fica no produto)

Independente da causa, estes contadores ficam no firmware, expostos na tela
de sistema e persistidos:

| Métrica | Para quê |
|---|---|
| flushes por `update()`, por evento | detectar regressão de custo de render |
| duração de `update()` (p50/p95/máx) | idem |
| espera de flush/PPA | separar render lento de transferência lenta |
| underruns de DSI, se o driver expuser | prova objetiva de banda |
| watermark de heap interno e PSRAM | pressão de memória |
| erases de flash por hora | violação do orçamento |

Regressão de render vira **falha de gate**, não descoberta em bancada seis
meses depois.

---

## 4. Ordem de execução

Cada passo é curto e ou elimina uma hipótese ou aponta uma causa.

```text
1. Ler e logar o endereço-base do draw buffer         (sem reflash de teste)
   └─ addr % 64 != 0 ?  →  hipótese 2.1 viva
2. Backlight travado em nível constante               (1 linha)
   └─ artefato some ?   →  causa é backlight; fim
3. Build "torture" só de render                       (§3.2)
   └─ reproduz ?        →  render/plataforma; segue 4
   └─ não reproduz ?    →  religa 1 subsistema por vez
4. Carimbo na tela + vídeo + log correlacionado       (§3.1)
   └─ agora o defeito tem frame, área e evento
5. Aplicar a correção que a evidência indicar         (uma por vez)
6. Confirmar com o mesmo método que produziu a evidência
```

**Critério de saída da Onda 0:** o glitch tem **causa nomeada, mecanismo
explicado e correção que o método da §3 confirma**. Não é "não vi mais
acontecer".

---

## 5. O que não fazer

- **Não trocar duas variáveis no mesmo reflash.** Foi assim que cinco
  tentativas viraram cinco resultados inconclusivos.
- **Não declarar resolvido por ausência de observação casual.** O sintoma
  já ficou minutos sem aparecer e voltou.
- **Não construir tela nova antes da Onda 0 fechar.** Código novo por cima
  de um defeito não atribuído só aumenta o espaço de busca — foi exatamente
  o que aconteceu entre os dois baselines anteriores.
- **Não pedir soak como prova.** O hardware acumula meses sem falha; o
  defeito é de firmware e reproduz em minutos quando se maximiza flush.
