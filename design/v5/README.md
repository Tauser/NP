# NovaPanel — Design System v5 (referência)

> **Esta pasta é referência de design, não código de produto.** Nada aqui
> entra no binário. O firmware vive em `firmware/components/ui/`; se os
> dois divergirem, **o firmware vence** e este diretório está com bug.
>
> Estado do projeto: só `docs/STATUS.md`.

## O que é

Modelos LVGL das telas do NovaPanel, escritos em C puro contra a mesma
grade, os mesmos tokens e as mesmas regras de custo de render que o
firmware usa. Serve para três coisas:

1. **Revisar layout sem placa.** Os mockups SVG em `mockups/` saem dos
   mesmos números da grade — dá para discutir composição sem flashar.
2. **Provar o padrão antes de migrar.** Uma tela nova nasce aqui, é
   aprovada, e só então vira `firmware/components/ui/src/screens/*.cpp`.
3. **Documentar o que ainda não existe.** Metade das telas aqui é
   aspiracional (ver tabela) — elas existem para tornar visível o custo de
   "completar" o produto, não para serem portadas às cegas.

## Estrutura

```text
core/
  np_tokens.h        cor por papel, tipografia, espaço, grade 12x8
  np_styles.h/.c     lv_style_t compartilhados (um por papel)
  np_components.h/.c primitivas e compostos; único lugar com lv_obj_set_style_*
screens/
  np_screens.h       declarações + catálogo
  np_<tela>.c        um arquivo por tela (12)
  np_catalog.c       ordem de geração/revisão
mockups/             SVG 1024x600 (16 vistas, geradas)
images/              PNG das mesmas 16 vistas
tools/
  gen_mockups.py     gera os SVG a partir da grade
  check.sh           gate: sintaxe + coerência da grade com o firmware
```

**12 telas, 16 vistas.** Setup e o painel lateral têm mais de um estado que
vale revisar separadamente, então rendem mais de um mockup cada:

| Tela (C) | Vistas (mockup) |
|---|---|
| `np_setup.c` | `setup_1`, `setup_password`, `setup_2`, `setup_3` |
| `np_sheets.c` | `sheet_wifi`, `sheet_profile` |
| demais | 1 vista cada |

**Os mockups são a especificação visual; os `.c` são o esqueleto
estrutural.** Os modelos em C demonstram os componentes, a grade e as
regras de custo — eles não reproduzem cada rótulo e cada pixel do mockup.
Quando os dois discordarem sobre *aparência*, o mockup manda; quando
discordarem sobre *como construir* (componente, geometria congelada,
escrita guardada), o `.c` manda.

## Correspondência com o firmware

| v5 (referência, C) | firmware (produto, C++) |
|---|---|
| `core/np_tokens.h` | `include/ui_tokens.hpp` + `include/ui_grid.hpp` |
| `core/np_styles.*` | `include/ui_styles.hpp` + `src/ui_styles.cpp` |
| `core/np_components.*` | `include/ui_components.hpp` + `src/ui_components.cpp` |
| `screens/np_home.c` | `src/screens/home_screen.cpp` |

Duas diferenças **deliberadas**, não acidentais:

- **Fontes.** Aqui os papéis apontam para as Montserrat embutidas do LVGL,
  para compilar em qualquer lugar sem asset. No firmware apontam para o
  catálogo real. O catálogo subsetado por papel (§5 do
  `UI-LAYOUT-SYSTEM.md`) ainda não existe em nenhum dos dois.
- **Geometria.** Os modelos aqui usam a grade cheia 1024×600. No firmware,
  Home e Mercado ainda entram atrás do chrome do shell (rail + topbar), num
  canvas menor — `ui_grid::kInset*`. Enquanto a navegação por gesto não
  substituir o rail, os mockups desta pasta mostram a tela **como ela será**,
  não como está.

## Telas

| Tela | Arquétipo | No firmware? | Observação |
|---|---|---|---|
| Boot | Fluxo | sim | 5 estágios de estado real |
| Agora (Home) | Hero | sim | relógio + BTC; agenda e vento/umidade/UV **ainda não** |
| Mercado | Hero | sim | BTC em foco + USD/BRL; abas e Ibovespa **ainda não** |
| Setup | Fluxo | sim | 3 passos, linhas de rede recicladas |
| Clima | Hero | parcial | só temperatura/resumo têm provider |
| Timer | Hero | **não** | sem serviço de timer |
| Agenda | Lista | **não** | sem serviço de calendário (Fase 8) |
| Alarmes | Lista | **não** | sem serviço nem persistência |
| Notificações | Lista | **não** | histórico; o alerta em si é toast do shell |
| Casa | Grade | **não** | automação é fase futura |
| Configurações | Grade | **não** | tarefas existentes marcadas em cor normal |
| Painel lateral | Fluxo | **não** | sobreposição, nunca cena de navegação |

O mockup mostra o produto **completo**, inclusive dado que hoje não existe
(Ibovespa, agenda, vento/umidade/UV, sensores de casa, uptime, temperatura
do SoC). Isso é proposital: serve para dimensionar o trabalho restante. Não
é autorização para desenhar essas áreas no firmware antes de existir
provider — `docs/UX_LAYOUT_REDESIGN.md` §2.

**Uma tela marcada "não" não deve virar firmware antes de existir fonte de
dado aprovada.** Desenhar interface para dado inexistente é proibido por
`docs/UX_LAYOUT_REDESIGN.md` §2 — foi por isso que a Home real não tem o
bloco de agenda que o mock original mostrava.

## As regras que este código existe para demonstrar

O contrato completo está em `docs/UI-LAYOUT-SYSTEM.md` §4. Em uma frase:

> **Geometria é decidida no `build()`. `update()` só troca conteúdo dentro
> de uma caixa que já tem tamanho e posição finais.**

As que mais aparecem aqui:

- **R1/R2** — largura explícita e `LV_LABEL_LONG_MODE_CLIP` em todo label
  dinâmico; nunca `LV_SIZE_CONTENT` acima de conteúdo que muda.
- **R4** — sem `transform_*`. O relógio grande usa uma fonte grande, não
  uma fonte pequena escalada (que forçaria *draw layer* + segundo blit, e
  ainda sairia borrado).
- **R5** — sem sombra. "Glow" é retângulo chapado em opacidade baixa.
- **R7** — `np_set_text`/`np_set_*_color` comparam antes de escrever.
- **R8** — relógio com **um label por dígito**: um tique típico troca 1
  dígito, não 5.
- **R10** — nenhuma tela chama `lv_obj_set_style_*`; só `np_components.c`.

## Gerar os mockups

```bash
cd docs/design/v5
python3 tools/gen_mockups.py     # escreve mockups/*.svg
bash tools/check.sh              # gate de sintaxe + grade
```

`check.sh` **não** compila contra o LVGL real (não há toolchain aqui); ele
confere sintaxe, a aritmética da grade e a ausência de violação de R5/R10.
Compilação de verdade acontece no firmware, via `idf.py build`.
