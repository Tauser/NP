/*
 * NovaPanel v5 -- estilos compartilhados (camada 2).
 *
 * Um lv_style_t estatico por papel, inicializado UMA vez. O objeto guarda
 * um ponteiro em vez de um array de propriedades locais -- e a diferenca
 * entre ~10 alocacoes pequenas por widget e uma so por papel
 * (UI-LAYOUT-SYSTEM.md SS1.3.2: essas alocacoes caem em SRAM interna com
 * CONFIG_SPIRAM_MALLOC_ALWAYSINTERNAL, disputando com o handshake TLS).
 *
 * Trocar tema = alterar estes estilos + lv_obj_report_style_change(NULL).
 */

#ifndef NP_STYLES_H
#define NP_STYLES_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Idempotente. Qualquer np_st_*() ja chama isto por dentro. */
void np_styles_init(void);

const lv_style_t *np_st_screen(void);    /* fundo full-bleed da cena */
const lv_style_t *np_st_surface(void);   /* card: borda 1px + raio de superficie */
const lv_style_t *np_st_raised(void);    /* card em destaque */
const lv_style_t *np_st_hairline(void);  /* separador de 1px */
const lv_style_t *np_st_label(void);     /* texto sem bg/borda */
const lv_style_t *np_st_tile(void);      /* tile de acao (arquetipo Grade) */
const lv_style_t *np_st_tile_on(void);   /* tile ligado */
const lv_style_t *np_st_row(void);       /* linha de lista (arquetipo Lista) */

#ifdef __cplusplus
}
#endif

#endif /* NP_STYLES_H */
