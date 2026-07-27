# AGENTS.md — ponto de entrada obrigatório

Leia este arquivo inteiro antes de qualquer alteração. Ele é curto de
propósito.

## 1. Antes de tocar em qualquer coisa

1. Leia [`STATUS.md`](STATUS.md). É a única fonte de estado.
2. **Não confie em resumo de conversa nem em lista de tarefas marcada como
   concluída.** Verifique no disco: `git status`, `git log --oneline -5` e
   leitura direta dos arquivos citados. Já aconteceu de uma sessão inteira
   descrever trabalho que nunca chegou ao repositório — e a sessão seguinte
   apagar código bom acreditando no resumo.
3. Só então planeje.

## 2. Regras que não se negociam

| # | Regra | Por quê |
|---|---|---|
| 1 | UI não faz request, não persiste e não toca hardware. Lê estado, publica intenção. | `docs/ARCHITECTURE.md` §4 |
| 2 | Toda mutação de estado passa pelo `StateStore`; sinalização pelo `EventBus`. | idem §3 |
| 3 | Só a `lvgl_task` toca objetos LVGL. Acesso externo exige o lock semântico do `board/`. | idem §6 |
| 4 | Todo request externo passa pelo orquestrador e roda no worker único: **1 HTTPS por vez**. | `docs/RESOURCE-BUDGET.md` §5 |
| 5 | Hardware só atrás de `IBoard`. API externa só atrás de interface de provider. `main/` é wiring: sem lógica, sem driver. | `docs/ARCHITECTURE.md` §4–5 |
| 6 | Tela entra pelo registro de spec — nunca por `if/else` no `main`. | `docs/UI-PATTERN.md` |
| 7 | Campo tocado por mais de uma task tem dono declarado no header, ou é atômico, ou trafega por fila. "Parece seguro" não é análise. | `docs/ARCHITECTURE.md` §6 |
| 8 | Alocação segue o `docs/RESOURCE-BUDGET.md`. Feature nova declara custo **antes** de entrar. | contrato físico |
| 9 | Dado crítico funciona offline, com `stale` explícito. Erro vira degradação clara, nunca travamento. | `docs/PRODUTO.md` |
| 10 | Mudou o desenho? Registre um ADR em `docs/DECISIONS.md` no mesmo commit. | rastreabilidade |

## 3. Proibições com histórico de dano

Estas já quebraram o produto em campo. Detalhes em
[`docs/PATRIMONIO-TECNICO.md`](docs/PATRIMONIO-TECNICO.md) §5.

- `static std::function` global (estoura `__cxa_atexit` → congela o boot).
- `abort()` em falha de display (vira boot loop quente).
- Múltiplos handshakes TLS simultâneos (~130 KB cada; esgota SRAM interna).
- Escrita de flash em rajada durante render.
- Adicionar campo a `AppState` ou a view-model sem refazer a conta de pilha
  da task que renderiza.
- Sombra (`shadow_width`) e `transform_*` em widget que se atualiza.

## 4. Definition of Done

Toda mudança, sem exceção:

1. Compila no alvo (`idf.py build`) **e** no host (`host_check.sh --app --tests`).
2. Testes nativos passam. Mudança em provider exige fixture de payload.
3. Nenhuma regra da §2 furada sem ADR novo.
4. `STATUS.md` atualizado se o estado mudou.
5. Nada de `build/`, `sdkconfig` gerado, temporários ou segredo no commit.
6. **O que não foi verificado está escrito como não verificado.** Se você
   não rodou `idf.py build`, diga isso — no commit e no STATUS.

## 5. Convenções de código

- C++17, estilo ESP-IDF, namespace `nova`. Sem exceções; erros por
  `Result<T>`/`Status`.
- Headers em `include/`, fontes em `src/`, um par `.hpp/.cpp` por unidade.
- Tipos `PascalCase`, funções e variáveis `snake_case`, membros com
  `sufixo_`.
- Objetos de vida longa no `app_main` (estáticos), passados por referência.
  Sem `new`/`delete` cru.
- Comentário explica **por quê**, com número quando houver medição. Comentário
  que repete o código é ruído.
- Um `kTag` por arquivo para log. Nunca logar segredo.

## 6. Ambiente

- Este repositório costuma ser operado por agentes a partir de um mount
  Windows. `git` pode falhar com `Operation not permitted` em deleções e em
  escrita de objetos; nesse caso libere a deleção da pasta antes de insistir,
  e prefira rodar `git` na máquina local se persistir.
- Não existe toolchain ESP-IDF em sandbox de agente. Validação possível ali:
  `host_check.sh` (g++ com shims) e checagens estáticas. `idf.py build` e
  bancada são do humano — **e devem ser declarados como não feitos** quando
  não forem feitos.
