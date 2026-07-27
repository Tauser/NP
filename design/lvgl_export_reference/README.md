# NovaPanel — LVGL v9.5 Export  (v2)

Código C completo para **LVGL v9.5**, atualizado para corresponder ao protótipo v2.

## Regra de uso

Este diretório é a **fonte visual de verdade** para recriar telas no firmware.
Quando uma tela for refeita ou portada, consulte primeiro o arquivo equivalente
a ela neste export antes de escrever código em `firmware/`.

Mapeamento de referência:

| Tela no firmware | Arquivo de referência |
|---|---|
| Boot | `screens/screen_boot.c` |
| Setup / Onboarding | `screens/screen_setup.c` |
| Home / Início | `screens/screen_home.c` |
| Agenda | `screens/screen_agenda.c` |
| Mercado | `screens/screen_mercado.c` |
| Casa | `screens/screen_casa.c` |
| Alarmes | `screens/screen_alarmes.c` |
| Clima | `screens/screen_clima.c` |
| Timer / Pomodoro | `screens/screen_timer.c` |
| Configurações | `screens/screen_config.c` |
| Notificações | `screens/screen_notif.c` |

Se uma tela do firmware ainda não tiver equivalente visual aqui, use o arquivo
mais próximo como base e registre o novo artefato neste mesmo diretório para que
ele vire a referência oficial da próxima recriação.

---

## Novidades nesta versão

| Mudança | Detalhe |
|---|---|
| **Tela de Boot** | `screen_boot.c` — splash animado, barra de progresso, botão "Pular →" |
| **Setup Wizard** | `screen_setup.c` — 4 passos: Nome → Wi-Fi → Fuso/Formato → Confirmação |
| **Rail icon-only** | Labels removidos dos botões de navegação; botões 44×44 px quadrados |
| **Topbar 54→60 px** | Maior área de toque para 7" |
| **Dot strip em fluxo** | `np_dots_bar` abaixo do conteúdo (não `position:absolute`) — sem sobreposição |
| **Fontes 7"** | `NP_F_XS` 10→12 px, `NP_F_SM` 12→14 px |
| **`np_phase_t`** | Novo enum `BOOT → SETUP → MAIN` para controle de fase |

---

## Estrutura

```
novapanel_theme.h          — cores, dimensões, helpers inline
novapanel.h                — enums, state struct, protótipos
novapanel.c                — init, rail (icon-only), topbar, dots, navegação
screens/
  screen_boot.c            — tela de boot (splash + barra animada)
  screen_setup.c           — wizard de configuração inicial
  screen_home.c            — Início
  screen_agenda.c          — Agenda
  screen_mercado.c         — Mercado (candlestick)
  screen_casa.c            — Casa (IoT)
  screen_alarmes.c         — Alarmes
  screen_clima.c           — Clima
  screen_timer.c           — Pomodoro
  screen_config.c          — Configurações
  screen_notif.c           — Notificações
CMakeLists.txt
```

---

## Display alvo

| Parâmetro | Valor |
|---|---|
| Resolução | 1024 × 600 px |
| Tamanho | **7 polegadas** |
| Profundidade | 16-bit RGB565 |
| LVGL | **v9.5.x** |
| Chip | ESP32-P4 |

---

## lv_conf.h — fontes necessárias

```c
#define LV_FONT_MONTSERRAT_12  1   /* mínimo para 7" */
#define LV_FONT_MONTSERRAT_14  1
#define LV_FONT_MONTSERRAT_16  1
#define LV_FONT_MONTSERRAT_20  1
#define LV_FONT_MONTSERRAT_24  1
#define LV_FONT_MONTSERRAT_28  1
#define LV_FONT_MONTSERRAT_32  1
#define LV_FONT_MONTSERRAT_48  1
#define LV_FONT_DEFAULT        (&lv_font_montserrat_14)
```

---

## Integração — sequência de boot

```c
#include "novapanel.h"

/* ── FreeRTOS boot task ────────────────────────────────────────── */
static void boot_task(void *arg)
{
    for (uint8_t pct = 0; pct < 100; ) {
        pct += (uint8_t)(esp_random() % 12 + 4);
        if (pct > 100) pct = 100;

        /* update bar (must be called from LVGL task or with mutex) */
        if (lv_display_get_default()) {
            lv_lock();
            lv_bar_set_value(g_boot_bar, pct, LV_ANIM_ON);
            np_boot_msg_update();   /* atualiza "Inicializando..." etc. */
            lv_unlock();
        }

        np_boot_advance(pct);
        vTaskDelay(pdMS_TO_TICKS(160));
    }

    vTaskDelay(pdMS_TO_TICKS(500));

    lv_lock();
    np_boot_finish();   /* mostra o wizard de setup */
    lv_unlock();

    vTaskDelete(NULL);
}

/* ── app_main ──────────────────────────────────────────────────── */
void app_main(void)
{
    /* inicializar display, touch e LVGL */
    bsp_display_start();
    bsp_touch_start();
    lv_init();
    /* ... driver setup ... */

    lv_lock();
    np_init();          /* constrói UI; boot overlay visível */
    lv_unlock();

    xTaskCreate(boot_task, "boot", 4096, NULL, 5, NULL);

    /* LVGL loop */
    while (1) {
        lv_lock();
        lv_timer_handler();
        lv_unlock();
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}
```

---

## Setup wizard — navegação manual

Se preferir controlar o wizard por toque sem os botões embutidos:

```c
/* avançar passo */
np_setup_next();

/* voltar passo */
np_setup_back();
```

---

## Navegação entre telas principais

```c
np_navigate_to(NP_SCR_AGENDA);
np_navigate_to(NP_SCR_MERCADO);
np_navigate_to(NP_SCR_CONFIG);
```

---

## Diferenças de API v8 → v9.5 aplicadas

| v8 | v9.5 |
|---|---|
| `lv_scr_act()` | `lv_screen_active()` |
| `lv_btn_create()` | `lv_button_create()` |
| `lv_obj_set_flex_grow(o,n)` | `lv_obj_set_style_flex_grow(o,n,0)` |
| `lv_draw_ctx_t *` | `lv_layer_t *` |
| `lv_event_get_draw_ctx()` | `lv_event_get_layer()` |
| `lv_draw_line(ctx,&d,&p1,&p2)` | `d.p1=p1;d.p2=p2;lv_draw_line(layer,&d)` |
| `lv_point_t` (draw) | `lv_point_precise_t` |
| `lv_obj_set_style_arc_rounded()` | `arc_round_start()` + `arc_round_end()` |
| `LV_LABEL_LONG_DOT` | `LV_LABEL_LONG_DOTS` |
