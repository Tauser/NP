# NovaPanel — Contrato de tela

> Documento-dono do padrão de telas, invalidação e view-model.
> Composição visual e custo de render: `UI-LAYOUT-SYSTEM.md`.
> Não afirma estado — ver `STATUS.md`.

## 1. Conceitos

```text
ScreenRegistry ──registra──> ScreenSpec (1 por tela)
Shell (chrome)  ──itera o registro; não conhece tela concreta
ViewModel       ──AppState → struct de apresentação pronta
```

## 2. `ScreenSpec`

```cpp
struct ScreenSpec {
    ScreenId    id;
    const char* title;
    uint32_t    invalidation_mask;          // OR de bits de evento (§3)
    lv_obj_t* (*build)(lv_obj_t* parent);   // cria widgets, devolve raiz
    void      (*update)(const AppState&);   // só troca conteúdo
    void      (*on_enter)();                // opcional
    void      (*on_leave)();                // opcional
};
```

Regras:

- **Tela nova = 1 arquivo + 1 linha de registro. Zero edição em `main/` ou
  no shell.**
- `build`/`update` rodam sempre sob o lock da UI, chamados pelo shell. A
  tela nunca adquire lock por conta própria.
- Handles ficam em struct de contexto **de escopo de arquivo**, com tipos
  triviais. `static std::function` global é proibido (congela o boot —
  `PATRIMONIO-TECNICO.md` §5).
- `update()` é idempotente e barato. Alvo: **≤ 4 flushes e ≤ 16 ms (p95)**.

## 3. Invalidação por máscara

- Cada tipo de evento relevante para UI tem um bit.
- A tela declara em `invalidation_mask` o que a invalida.
- O shell drena o `UiDispatcher` (dono único do coalescing), acumula a
  máscara do ciclo e chama `update()` **uma vez** por tela suja **visível**.
- Tela invisível suja marca dirty e repinta em `on_enter`.
- Eventos com payload individual (navegação, toast) têm tratamento próprio
  no shell — são os **únicos** casos especiais permitidos.

Evento novo = definir o bit; telas interessadas somam à máscara. **Nenhum
mapa central para editar.**

## 4. View-model

- Builder puro por tela: `make_<tela>_vm(const AppState&) -> <Tela>Vm`, sem
  LVGL, testável no host.
- O view-model entrega **strings já formatadas**, cores já decididas e flags
  de `stale`/ausência já resolvidas.
- `update()` só transfere `Vm → widgets`. **Formatar dentro de `update()` é
  code smell**; calcular regra de negócio dentro do builder também — ele
  formata, não decide domínio.
- Strings de produto em tabela única (`strings_ptbr.hpp`).

### 4.1 Custo de pilha

O view-model é construído na pilha da task que renderiza. Campo novo custa
**pilha**, não só RAM (`RESOURCE-BUDGET.md` §2.7). Regra prática: campo sem
consumidor não entra; campo que só existe "para o futuro" não entra.

## 5. Shell

Dono de: navegação, chrome do topo, indicador de cena, overlay de
notificação, toast e teclado compartilhado.

Navegação: o shell publica intenção → estado → evento → shell troca a tela
ativa e chama `on_leave`/`on_enter`/`update`.

Construção de tela é **preguiçosa** na primeira navegação, exceto Boot e a
tela inicial — controla o pico de RAM de widgets.

## 6. Interação

- Callback de widget **publica intenção na fila**; nunca executa IO, NVS ou
  lógica de domínio.
- Exceção documentada: preview contínuo de hardware barato (ex.: brilho
  durante o arraste) pode chamar a HAL direto; **persistência só no
  released**.
- Feedback visual/sonoro imediato é permitido. Efeito de estado, não.

## 7. Ciclo de vida

Toda tela declara sua classe:

| Classe | Comportamento |
|---|---|
| `Resident` | construída no boot, nunca destruída (tela inicial) |
| `Cached` | preguiçosa no primeiro acesso, mantida |
| `Transient` | destruída em `on_leave`; contexto zerado |

Sem isso, visitar todas as telas uma vez deixa todas as árvores de widget
residentes em SRAM interna para sempre.

## 8. Checklist de revisão (tela nova)

1. Arquivo único em `screens/` + builder de view-model + 1 linha de registro.
2. `invalidation_mask` **mínima** — só os eventos que realmente usa.
3. Nenhum include de service, provider ou board no arquivo da tela.
4. Builder com teste de host cobrindo **válido, stale e ausente**.
5. Nenhum dado desenhado sem fonte real no estado (`PRODUTO.md` §4).
6. Arquivo ≤ 200 linhas (`ARCHITECTURE.md` §11).
7. Custo de `update()` medido e dentro do orçamento.
