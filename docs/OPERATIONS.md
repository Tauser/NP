# NovaPanel — Operação

> Build, flash, release e runbooks. Não afirma estado — ver `STATUS.md`.

## 1. Ambiente

```text
ESP-IDF     v5.5.x
Alvo        esp32p4
Porta       usar a "USB1.1 FS" (USB nativo/JTAG-Serial), não a ponte CH343
            Windows: COMx   ·   Linux: /dev/ttyACM0 (ACM, não USB)
```

**VSCode:** abrir o `NovaPanel.code-workspace`, não a pasta — o projeto IDF
é `firmware/`, e a extensão procura o `CMakeLists.txt` na raiz da pasta
aberta. A configuração do IDF vive em `firmware/.vscode/settings.json`.

**O alvo não é uma configuração.** Não existe setting de alvo desde a v1.9.0
da extensão (o antigo `idf.adapterTargetName` foi removido). O alvo vive no
`sdkconfig`; a barra de status mostra `esp32` enquanto não houver um. Rode
`idf.py set-target esp32p4` uma vez — o `sdkconfig.defaults` já traz
`CONFIG_IDF_TARGET="esp32p4"`, então o `sdkconfig` nasce correto.

**`idf.enableIdfComponentManager` precisa ser `true`.** O default da
extensão é `false`, e este projeto declara dependências em
`main/idf_component.yml` (LVGL, BSP, ESP-Hosted). Com `false`, elas nunca
são baixadas e o build falha em include não encontrado.

## 2. Build de desenvolvimento

```bash
cd firmware
idf.py set-target esp32p4
idf.py build
idf.py -p <porta> flash monitor
```

Antes de commitar, sempre:

```bash
bash tools/scripts/host_check.sh --app --tests
bash tools/scripts/arch_check.sh
bash tools/scripts/size_check.sh
bash tools/scripts/ui_check.sh
```

`host_check.sh` é **portátil** (bash puro + g++ com shims). Se falhar só no
seu ambiente, o bug é do script — corrija o script, não pule a validação.

## 3. Perfis de build

| Perfil | Para quê | Características |
|---|---|---|
| `dev` | dia a dia | log em INFO, coredump ligado, sem criptografia |
| `bench` | medição | dev + instrumentação de render verbosa |
| `prod` | unidade final | log em WARN, coredump **ligado**, Secure Boot + criptografia de flash |

Coredump fica ligado **inclusive em produção**: triagem de campo depende
dele, e desligá-lo já criou contradição operacional no passado.

## 4. Provisionamento de unidade

> **Procedimento irreversível.** Criptografia de flash em modo release e
> Secure Boot **não voltam atrás**. Ensaiar em unidade sacrificável antes.

Pré-requisitos, sem exceção:

1. Fluxo OTA funcionando, com assinatura verificada e **rollback automático
   já testado** (`SECURITY.md` §4). Sem isso, a unidade fica sem caminho de
   atualização.
2. Chaves geradas e guardadas fora do repositório.
3. Partição de coredump apagada.
4. Anti-rollback só é ligado **depois** de o OTA estar provado.

## 5. Runbooks

### 5.1 Painel não liga (tela apagada)

1. Serial mostra boot? Se não → alimentação/flash.
2. Se o log passa de "display initialized" mas a tela fica escura →
   **backlight**. Ele só liga depois do primeiro frame (`HARDWARE.md`); se
   o primeiro render falhou, o backlight nunca sobe.
3. Breadcrumb de retry no NVS indica quantas tentativas de display houve.

### 5.2 Boot loop

1. Ler o motivo do reset e o coredump.
2. Padrões conhecidos (`PATRIMONIO-TECNICO.md` §5): estouro de pilha na task
   que renderiza; `static std::function` global; inicialização de áudio com
   ponteiro nulo.
3. Coredump corrompido **junto com** boot loop é assinatura conhecida de
   falha na inicialização do áudio.

### 5.3 Sem rede

1. O link com o C6 subiu? Falha ali é falha de rede do produto.
2. Hora sincronizada? **Sem hora plausível o HTTPS falha** por certificado
   (`ADR-015`).
3. Breaker aberto? A tela de sistema mostra o estado por domínio.
4. Sem rede, o painel **deve** seguir operável com cache marcado como
   `stale`. Se travou, é bug de degradação, não de rede.

### 5.4 Artefato visual

Não improvisar: seguir `GLITCH-PROTOCOLO.md`. Em ordem — endereço do draw
buffer, backlight, build "torture", correlação por vídeo. **Uma variável por
reflash.**

### 5.5 Dado velho na tela

1. O valor está marcado como `stale`? Se sim, é comportamento correto.
2. Se está marcado como ao vivo e não é, o bug é de `source`/`last_update`
   no service — não da UI.

## 6. Atualização do firmware do C6

Via Slave OTA por SDIO (método "Partition OTA" do exemplo
`host_performs_slave_ota`). A partição `c6_ota` já está reservada. O header
"ESP32-C6 UART Terminal" é plano B **não testado**.

## 7. Higiene de repositório

Nunca commitar: `build/`, `sdkconfig` gerado, `managed_components/`,
`__pycache__/`, coredump, log de bancada bruto, chave ou segredo.

O `sdkconfig.defaults` **é** versionado — é ele que carrega as receitas de
plataforma do `PATRIMONIO-TECNICO.md` §1.
