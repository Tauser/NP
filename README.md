# NovaPanel — baseline 2026-07

Smart display pessoal **local e offline-first** para a placa Waveshare
ESP32-P4-WIFI6-Touch-LCD-7B (7", 1024×600, MIPI-DSI). Firmware em ESP-IDF +
LVGL, C++17.

O painel é uma peça de informação de parede: calmo quando consultado de
relance, direto quando tocado, e **útil sem internet**.

---

## Regra zero — onde está a verdade

- **Estado atual: somente [`STATUS.md`](STATUS.md).** Nenhum outro arquivo
  deste repositório — inclusive este — afirma o que está pronto. Se algum
  contradisser o STATUS, o STATUS vence e o outro está com bug.
- Todo fechamento de etapa atualiza o `STATUS.md` **no mesmo commit**.

Essa regra existe porque a causa-raiz da degradação do baseline anterior
não foi técnica: foi documentação afirmando estados divergentes num repo
operado por agentes. Divergência de documentação tem prioridade de crash.

## Navegação

| Documento | Responde |
|---|---|
| [`STATUS.md`](STATUS.md) | O que está pronto **agora** |
| [`AGENTS.md`](AGENTS.md) | Ponto de entrada obrigatório para agentes |
| [`docs/PRODUTO.md`](docs/PRODUTO.md) | O que o produto é e o que deliberadamente não é |
| [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) | Camadas, fluxo de dados, concorrência |
| [`docs/HARDWARE.md`](docs/HARDWARE.md) | Fatos da placa validados em bancada |
| [`docs/RESOURCE-BUDGET.md`](docs/RESOURCE-BUDGET.md) | Contrato físico: memória, banda, flash, rede |
| [`docs/PATRIMONIO-TECNICO.md`](docs/PATRIMONIO-TECNICO.md) | Tudo que já custou caro para descobrir |
| [`docs/GLITCH-PROTOCOLO.md`](docs/GLITCH-PROTOCOLO.md) | O defeito que bloqueia o produto e como atribuí-lo |
| [`docs/UI-PATTERN.md`](docs/UI-PATTERN.md) | Contrato de tela |
| [`docs/UI-LAYOUT-SYSTEM.md`](docs/UI-LAYOUT-SYSTEM.md) | Grade, componentes e custo de render |
| [`docs/DECISIONS.md`](docs/DECISIONS.md) | ADRs |
| [`docs/TESTING.md`](docs/TESTING.md) | Pirâmide de teste e gates |
| [`docs/OPERATIONS.md`](docs/OPERATIONS.md) | Build, flash, release, runbooks |
| [`docs/SECURITY.md`](docs/SECURITY.md) | Secure Boot, criptografia, OTA, segredos |
| [`docs/ROADMAP.md`](docs/ROADMAP.md) | Ondas de entrega e critérios de saída |
| [`design/v5/`](design/v5/README.md) | Design system e mockups 1024×600 (referência visual, fora do build) |

## O que este baseline faz diferente

Este é o terceiro começo do projeto. Os dois anteriores tinham núcleo bom e
morreram por motivos que agora são contrato:

1. **O defeito de plataforma vem antes da funcionalidade.** Existe um
   artefato visual ("piscada") que sobreviveu a cinco correções e a uma
   reconstrução inteira. Nenhuma tela nova entra antes de ele estar
   **atribuído a uma causa medida** — ver `docs/GLITCH-PROTOCOLO.md`. Cinco
   tentativas falharam porque cada uma trocou uma variável sem instrumentar
   o mecanismo; isso não se repete aqui.
2. **Contrato estrutural nasce na primeira onda, não "depois".** HAL,
   registro de telas, interfaces de provider e propriedade de threads são
   Onda A. "Estrutura depois" nunca chegou nas duas tentativas anteriores.
3. **Gate é o que uma máquina consegue verificar.** Critério de saída que
   depende de alguém olhar a tela por sete dias não é gate — é esperança.
   Ver `docs/TESTING.md`.
4. **Conhecimento pago fica registrado.** `docs/PATRIMONIO-TECNICO.md`
   existe para que nenhuma descoberta cara precise ser feita duas vezes.

## Premium, aqui, significa previsibilidade

Não é quantidade de telas. É: liga rápido, nunca pisca, responde ao toque
na hora, continua útil sem rede, atualiza sem virar tijolo e conta a
verdade quando algo falha. As metas numéricas estão em
[`docs/ROADMAP.md`](docs/ROADMAP.md) §1 e todas são medíveis.

## Estrutura

```text
NovaPanel.code-workspace   abra este arquivo no VSCode
STATUS.md                  fonte única de estado
AGENTS.md                  ponto de entrada para agentes
PROMPT-INICIAL.md          prompt para iniciar o desenvolvimento
docs/                      documentação canônica
design/                    referência visual (design system + mockups)
firmware/                  o projeto ESP-IDF
tools/scripts/             gates de validação
```

`design/` é **referência**, não código de produto: nada ali entra no
binário. Quando design e firmware discordarem sobre aparência, o mockup
manda; sobre como construir, o firmware manda.

## Abrir no VSCode

**Abra o `NovaPanel.code-workspace`** (`File > Open Workspace from File...`),
não a pasta.

O projeto ESP-IDF é `firmware/`, não o repositório — que também guarda
`docs/`, `design/` e `tools/`. A extensão da Espressif trata a pasta aberta
como raiz do projeto e procura o `CMakeLists.txt` nela; abrindo só a pasta,
ela não acha o projeto. O arquivo de workspace declara as duas raízes e
resolve isso.

Onde cada configuração vive, e por quê:

| Arquivo | Contém |
|---|---|
| `NovaPanel.code-workspace` | as duas raízes + extensões recomendadas |
| `firmware/.vscode/settings.json` | tudo de `idf.*` (alvo, caminhos, porta, OpenOCD) |
| `firmware/.vscode/launch.json` | debug por JTAG |
| `.vscode/settings.json` | editor e clangd |
| `.vscode/tasks.json` | build, flash, monitor e gates |

`idf.*` não é duplicado na raiz de propósito: em workspace multi-root,
`${workspaceFolder}` fica ambíguo e a extensão passa a apontar para a pasta
errada.

Ajuste ao seu ambiente em `firmware/.vscode/settings.json`:
`idf.espIdfPathWin`, `idf.toolsPathWin` e `idf.portWin`.

### O alvo (esp32p4)

**Não existe setting de alvo.** O antigo `idf.adapterTargetName` foi
removido na v1.9.0 da extensão. O alvo vive no `sdkconfig`, e a barra de
status mostra `esp32` até existir um `sdkconfig` dizendo o contrário.

Primeira configuração — faça uma vez, pelo terminal:

```bash
cd firmware
idf.py set-target esp32p4
```

Isso gera o `sdkconfig` a partir do `sdkconfig.defaults` (que já traz
`CONFIG_IDF_TARGET="esp32p4"`). Depois disso a extensão passa a mostrar
`esp32p4`, e o comando `ESP-IDF: Set Espressif Device Target` funciona
normalmente.

> **Rode isso ANTES de deixar a extensão configurar sozinha.** O BSP da
> Waveshare só existe para `esp32p4`. Se o IDF configurar assumindo `esp32`,
> o resolvedor falha com uma mensagem enganosa —
> `no versions of waveshare/... match ==1.0.4` — que parece erro de versão
> mas é erro de alvo. Ver `docs/PATRIMONIO-TECNICO.md` §1.1.

Se o `set-target` falhar com **`exit code 2`** e
`Directory 'build' doesn't seem to be a CMake build directory`:

```bash
cd firmware
rm -rf build            # Windows: rmdir /s /q build
idf.py set-target esp32p4
```

Isso acontece quando um configure abortado deixa `build/` sem
`CMakeCache.txt`: o `fullclean` (do qual o `set-target` depende) se recusa a
apagar um diretório que não reconhece, e o ciclo trava. Apagar na mão
resolve.

## Build

```bash
cd firmware
idf.py build
idf.py -p <porta> flash monitor
```

Pelo VSCode: `Ctrl+Shift+B` (build) e a paleta de tarefas para flash,
monitor e gates.

Antes de commitar, sempre:

```bash
bash tools/scripts/check_all.sh
```

Detalhes e perfil PROD: [`docs/OPERATIONS.md`](docs/OPERATIONS.md).
