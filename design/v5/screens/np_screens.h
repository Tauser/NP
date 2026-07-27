/*
 * NovaPanel v5 -- modelos de tela (camada 4).
 *
 * Cada tela e um arquivo. Todas recebem `parent` e devolvem a raiz da
 * cena; os valores exibidos sao amostras representativas, porque esta
 * pasta e REFERENCIA VISUAL, nao codigo de produto (README.md). No
 * firmware, o equivalente recebe um view-model e nunca formata nada
 * dentro do update().
 *
 * Quatro arquetipos cobrem as treze telas (UI-LAYOUT-SYSTEM.md SS3.4):
 *
 *   Hero   -- 1 foco + apoio           Home, Market, Weather, Timer
 *   Lista  -- N linhas recicladas      Agenda, Alarms, Notifications
 *   Grade  -- tiles de acao            Devices, Settings
 *   Fluxo  -- full-bleed, um passo     Boot, Setup, Sheets
 *
 * As telas marcadas ASPIRACIONAL abaixo nao existem no produto: nao ha
 * servico, provider nem campo de estado que as alimente. Elas vivem aqui
 * como exploracao de layout e NAO devem virar firmware antes de existir
 * fonte de dado aprovada -- desenhar interface para dado inexistente e
 * proibido por docs/UX_LAYOUT_REDESIGN.md SS2.
 */

#ifndef NP_SCREENS_H
#define NP_SCREENS_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---- implementadas no firmware ---- */
lv_obj_t *np_boot_create(lv_obj_t *parent);          /* Fluxo */
lv_obj_t *np_home_create(lv_obj_t *parent);          /* Hero  */
lv_obj_t *np_market_create(lv_obj_t *parent);        /* Hero  */
lv_obj_t *np_setup_create(lv_obj_t *parent);         /* Fluxo */

/* ---- ASPIRACIONAL: sem fonte de dado hoje ---- */
lv_obj_t *np_weather_create(lv_obj_t *parent);       /* Hero  -- so temp/resumo existem */
lv_obj_t *np_timer_create(lv_obj_t *parent);         /* Hero  */
lv_obj_t *np_agenda_create(lv_obj_t *parent);        /* Lista -- ROADMAP Fase 8 */
lv_obj_t *np_alarms_create(lv_obj_t *parent);        /* Lista */
lv_obj_t *np_notifications_create(lv_obj_t *parent); /* Lista */
lv_obj_t *np_devices_create(lv_obj_t *parent);       /* Grade -- automacao, fase futura */
lv_obj_t *np_settings_create(lv_obj_t *parent);      /* Grade */
lv_obj_t *np_sheets_create(lv_obj_t *parent);        /* Fluxo -- painel lateral sobreposto */

/* Catalogo para o harness de mockup iterar. */
typedef struct {
    const char *id;
    const char *title;
    const char *archetype;
    lv_obj_t *(*create)(lv_obj_t *parent);
} np_screen_entry_t;

const np_screen_entry_t *np_screen_catalog(void);
int np_screen_catalog_count(void);

#ifdef __cplusplus
}
#endif

#endif /* NP_SCREENS_H */
