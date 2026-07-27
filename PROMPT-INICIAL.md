# Prompt inicial para iniciar o desenvolvimento

Copie o bloco abaixo numa sessão nova, com a pasta `D:\Projetos\NP`
conectada.

---

```text
Projeto: D:\Projetos\NP (NovaPanel — smart display ESP32-P4, ESP-IDF + LVGL).

ANTES DE QUALQUER COISA, leia nesta ordem:
1. AGENTS.md          — regras não-negociáveis e Definition of Done
2. STATUS.md          — única fonte de estado do projeto
3. docs/GLITCH-PROTOCOLO.md  — o defeito que bloqueia todo o roadmap
4. docs/ARCHITECTURE.md §4, §5 e §11 — HAL, concorrência e limites de tamanho

Verifique o estado no disco (git status, ls, leitura direta dos arquivos).
Não confie em resumo de conversa nem em lista de tarefas marcada como
concluída — esse erro já apagou código bom neste projeto.

CONTEXTO
O repositório tem documentação completa, scaffolding de VSCode/IDF, gates em
tools/scripts/ e um firmware que é só esqueleto: app_main loga uma linha e
retorna. Nenhum build foi executado ainda nesta máquina.

Existe um artefato visual ("piscada") herdado dos baselines anteriores que
sobreviveu a cinco correções. A ADR-003 torna a atribuição dele bloqueante:
NENHUMA tela de produto entra antes de o defeito ter causa nomeada. A
hipótese principal está em docs/GLITCH-PROTOCOLO.md §2.1 — o draw buffer do
LVGL é alocado com alinhamento de 4 bytes enquanto a linha de cache L2 do P4
tem 64, e esse buffer é origem de DMA.

SUA TAREFA — Onda 0, passo 1
Entregar o mínimo de firmware que permita atribuir o glitch:

1. components/board/ — HAL mínima e real, seguindo ARCHITECTURE.md §4:
   IBoard + WaveshareBoard + MockBoard, cobrindo display, lock_ui,
   lock_shared_i2c, set_brightness e rtc_unix_time_s. Sem driver no main/.
   Backlight só liga DEPOIS do primeiro frame.

2. Diagnóstico de boot, logado em nível INFO:
   - endereço-base e tamanho de cada draw buffer do LVGL, com o resto da
     divisão por 64 (CONFIG_CACHE_L2_CACHE_LINE_SIZE);
   - resultado de board::assert_dma_safe() para cada buffer (ADR-010);
   - alvo, revisão do silício, tamanho de PSRAM e heap interno livre.

   Se algum endereço já for múltiplo de 64 por sorte do alocador, a hipótese
   2.1 cai — e isso precisa ficar explícito no log, sem reflash extra.

3. Instrumentação permanente de render (docs/GLITCH-PROTOCOLO.md §3.3):
   contagem de flushes e duração por update, espera de flush/PPA,
   watermark de pilha da lvgl_task. Números, não adjetivos.

4. Modo "torture" atrás de flag de build (§3.2): só render, sem Wi-Fi, sem
   NVS, sem cache, alternando conteúdo a ~10 Hz para maximizar flush.

NÃO FAÇA AGORA
- Nenhuma tela de produto, nenhum design system, nenhuma fonte subsetada.
- Nenhum provider, service de dados, cache ou rede.
- Não tente corrigir o glitch. Esta etapa produz EVIDÊNCIA, não correção.

RESTRIÇÕES
- Render acontece na lvgl_task (ADR-011). Não copie AppState inteiro para a
  pilha — isso já causou stack protection fault em campo.
- A pilha da lvgl_task está declarada como "a determinar" em
  RESOURCE-BUDGET.md §2.1: meça a marca d'água e registre o número lá.
- Limites de tamanho do ARCHITECTURE.md §11 valem desde o primeiro arquivo.
- Uma variável por experimento. Nada de mudar duas coisas no mesmo reflash.

ANTES DE TERMINAR
- Rode: bash tools/scripts/check_all.sh
- Rode: cd firmware && idf.py build
- Atualize STATUS.md com o que foi feito e, separadamente, o que NÃO foi
  verificado. "Não verificado" é resposta obrigatória quando for o caso.
- Se você não puder rodar idf.py (sem toolchain), diga isso explicitamente
  em vez de presumir que compila.

Comece propondo o plano e o que vai medir. Não escreva código antes de eu
aprovar o plano.
```

---

## Por que o escopo é esse

O primeiro passo **não** é uma tela bonita, e isso é deliberado.

O projeto já tentou construir por cima de um defeito não atribuído duas
vezes, e as duas terminaram com mais código e o mesmo problema. A Onda 0
existe para quebrar esse ciclo, e ela precisa de firmware mínimo para
produzir evidência — display subindo, buffers logados, render instrumentado.
Nada além disso.

O passo mais barato de todos está embutido no item 2: **logar o endereço do
draw buffer**. Se ele já for múltiplo de 64, a hipótese principal morre em
cinco minutos, sem reflash de teste. Falsificar antes de corrigir é a
diferença entre este baseline e os anteriores.

A HAL entra junto porque é contrato de Onda A (`ARCHITECTURE.md` §4) e
porque o modo torture precisa de algo abaixo dele. Fazer um bring-up
descartável agora significaria jogar fora e refazer — exatamente o padrão
que este baseline foi escrito para evitar.

## Se preferir começar ainda menor

Trocar a tarefa por só os itens 2 e 3, com um bring-up direto de BSP sem
HAL. Entrega evidência mais rápido, ao custo de código que será refeito.
Recomendo a versão completa: a diferença é de horas, não de dias.
