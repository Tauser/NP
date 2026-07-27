# NovaPanel — Sistema de layout e custo de render

> Como compor tela sem violar o `RESOURCE-BUDGET.md`. Complementa o
> `UI-PATTERN.md` (contrato) com **composição** e **custo**.
> Não afirma estado — ver `STATUS.md`.

## 1. O princípio

> **Geometria é decidida no `build()`. O `update()` só troca conteúdo dentro
> de uma caixa que já tem tamanho e posição finais.**

Essa frase separa uma atualização de 2 flushes de uma de 13, e é
verificável estaticamente (§6).

## 2. Camadas

```text
4. Arquétipos    Hero, Lista, Grade, Fluxo          ui/layouts/
3. Componentes   Card, Metric, Pill, Clock, Spark   ui/components/
2. Grade + estilos compartilhados                   ui/ui_grid.*, ui_styles.*
1. Tokens        cor, tipografia, espaço, raio      ui/ui_tokens.hpp
```

Tela concreta consome as camadas 3 e 4 e **nunca** chama
`lv_obj_set_style_*` diretamente. Isso é gate, não sugestão (§6).

## 3. Tokens

Escalas fechadas — nada fora delas em código de tela:

```text
Espaço   4, 8, 12, 16, 24, 32, 48        (sem 18, sem 26, sem 30)
Raio     8 (controle), 20 (superfície), circular
Elevação cor de fundo + borda de 1 px — NUNCA sombra (§4, R5)
```

Cor por **papel semântico**, não por nome: `positive()`, não `green()`. É o
que permite trocar tema sem tocar em tela.

Tipografia por papel, com no máximo **6 fontes** no binário:

| Papel | Charset | Uso |
|---|---|---|
| `display_hero` | `0-9 :` | relógio |
| `display_lg` | dígitos + símbolos de unidade | valor em foco |
| `text_lg` / `text_md` / `text_sm` | Latin pt-BR | rótulo, corpo, caption |
| `icons` | ~20 codepoints | chrome e tendência |

## 4. Regras não-negociáveis de custo

Cada uma responde a um custo medido ou a um mecanismo do
`RESOURCE-BUDGET.md`.

**R1 — Geometria congelada após o `build()`.** Nenhum widget muda de
tamanho ou posição em `update()`. Todo label dinâmico recebe largura
explícita e modo de clip.
*Por quê:* mudar tamanho invalida o retângulo antigo **e** o novo, e dispara
relayout no pai.

**R2 — Tamanho por conteúdo proibido em ancestral de conteúdo dinâmico.**
Mesmo motivo, propagado árvore acima.

**R3 — Layout automático (flex/grid) só em contêiner estático.** Nada no
caminho de `update()`.

**R4 — Sem `transform_*`, sem opacidade em contêiner, sem blend mode em
widget que se atualiza.** Todos forçam *draw layer* — buffer intermediário
mais composição. Relógio grande usa **fonte grande**, não fonte pequena
escalada.

**R5 — Sombra proibida.** Com cache de sombra desligado, toda sombra é
recalculada a cada repintura **e** infla o retângulo sujo pelo spread.
"Glow" é retângulo chapado em opacidade baixa, ou imagem pré-renderizada.

**R6 — Orçamento de flush.** Alvo: **≤ 4 flushes e ≤ 16 ms (p95)** por
`update()`. Regiões que mudam juntas ficam na mesma banda do draw buffer;
nenhuma atualização lógica varre a altura inteira da tela.

**R7 — Guardar escrita de estilo, não só de texto.** Cor, opacidade e
pontos de linha passam por comparação prévia: LVGL invalida mesmo quando o
valor é idêntico.

**R8 — Cadência do relógio.** O relógio mostra `HH:MM`: muda **uma vez por
minuto**, ainda que o evento chegue a cada segundo. Duas consequências:
o shell filtra o evento por minuto renderizado; e o relógio usa **um label
por dígito** — um tique típico troca 1 dígito, não 5, e o fetch de glifo em
flash cai junto (`RESOURCE-BUDGET.md` §1.2).

**R9 — Ciclo de vida declarado por tela.** Ver `UI-PATTERN.md` §7.

**R10 — `lv_obj_set_style_*` proibido em arquivo de tela.** Só tokens,
estilos compartilhados e componentes.

## 5. Fontes e ilustrações

**Fonte é banda, não espaço.** Flash sobra; o custo é *locality* de glifo
(`RESOURCE-BUDGET.md` §1.2).

1. **Subsetar por papel**: `display_hero` precisa de dígitos e dois pontos;
   texto precisa de Latin pt-BR (~120 glifos), não do charset inteiro.
2. **Consolidar a escada** para 6 fontes. Remover as não usadas do build.
3. **Ícones**: subset de ~20 codepoints em um tamanho, não catálogo inteiro.
4. **Gate**: script lê o `.map`, soma os símbolos de fonte e falha acima do
   teto (`RESOURCE-BUDGET.md` §7).

**Ilustração como array bruto RGB565A8, desenhada direto do flash.** Sem
decoder, sem buffer de decode, **0 B de RAM**. PNG/JPEG em runtime são
proibidos.

## 6. Gate estático — `ui_check.sh`

Grep barato sobre `ui/screens/` e `ui/layouts/`, falhando em:

```text
lv_obj_set_style_transform_     (R4)
shadow_width                    (R5)
tamanho-por-conteúdo            (R2)
lv_obj_set_style_               (R10, exceto em styles/components)
lv_obj_create|lv_label_create   dentro de função update_*   (R1)
static std::function            (proibição de plataforma)
arquivo > 200 linhas            (ARCHITECTURE.md §11)
```

Não substitui revisão; impede a **regressão silenciosa**, que é como esses
custos voltaram no baseline anterior.

## 7. Grade

Grade fixa de 12 colunas × 8 linhas sobre 1024×600, resolvida em pixels no
`build()` — custo zero em runtime, ao contrário de flex/grid do LVGL:

```text
Horizontal  margem 64 │ 12 col × 60 px │ gutter 16
            64 + 12×60 + 11×16 + 64 = 1024   ✓
            col_x(k) = 64 + k×76      col_w(n) = 76n − 16

Vertical    margem 24 │ 8 linhas × 55 px │ gutter 16
            24 + 8×55 + 7×16 + 24 = 600      ✓
            row_y(k) = 24 + k×71      row_h(n) = 71n − 16
```

Tela declara **região**, não coordenada mágica. Com draw buffer de 60 linhas
e linha de grade de 71 px, uma região de 1 linha cruza no máximo 2 bandas —
o custo passa a ser previsível a partir da grade:

```text
flushes ≈ Σ ceil(altura da região suja / linhas do draw buffer)
          por região disjunta invalidada
```

## 8. Arquétipos

Onze telas não precisam de onze layouts. Precisam de quatro:

| Arquétipo | Estrutura | Regra de atualização |
|---|---|---|
| **Hero** | 1 foco + apoio + rodapé | cada bloco tem retângulo fixo e atualiza sozinho |
| **Lista** | header + **N linhas de altura fixa recicladas** | rolar/atualizar = re-textar linha existente; **proibido criar objeto por item** |
| **Grade** | header + tiles de ação | tile é estático; só rótulo de estado muda |
| **Fluxo** | full-bleed, um passo por vez | passo troca por mostrar/esconder, **nunca rebuild** |

A regra de linhas recicladas é o que impede que lista tenha consumo de RAM
proporcional ao volume de dados — o jeito mais fácil de estourar o
orçamento sem perceber.

## 9. Evidência exigida por tela

No PR, uma tabela medida em placa:

| Evento | Flushes | ms (p95) | Heap interno após `build()` |
|---|---|---|---|

Mais os testes de host do view-model (`UI-PATTERN.md` §8.4).
