/*
 * NovaPanel v5 -- tokens do design system (camada 1).
 *
 * REFERENCIA, NAO CODIGO DE PRODUTO. Ver docs/design/v5/README.md.
 * O equivalente compilado no firmware e
 * firmware/components/ui/include/ui_tokens.hpp (C++), que carrega os
 * mesmos valores. Divergiu? o firmware vence e este arquivo esta com bug.
 *
 * Contrato: docs/UI-LAYOUT-SYSTEM.md SS3.1 (tokens) e SS3.2 (grade).
 */

#ifndef NP_TOKENS_H
#define NP_TOKENS_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------ */
/* Tela                                                                */
/* ------------------------------------------------------------------ */

#define NP_SCREEN_W 1024
#define NP_SCREEN_H 600

/* ------------------------------------------------------------------ */
/* Grade 12 x 8 (UI-LAYOUT-SYSTEM.md SS3.2)                             */
/*                                                                      */
/*   64 + 12*60 + 11*16 + 64 = 1024                                     */
/*   24 +  8*55 +  7*16 + 24 =  600                                     */
/*                                                                      */
/* Resolvida em pixels no build(); custo zero em runtime (nao usa       */
/* flex/grid do LVGL, que recalculam layout a cada invalidacao).        */
/* ------------------------------------------------------------------ */

#define NP_MARGIN_H 64
#define NP_COL_UNIT 60
#define NP_GUTTER_H 16

#define NP_MARGIN_V 24
#define NP_ROW_UNIT 55
#define NP_GUTTER_V 16

#define NP_COL_X(col)     (NP_MARGIN_H + (col) * (NP_COL_UNIT + NP_GUTTER_H))
#define NP_COL_W(span)    ((span) * (NP_COL_UNIT + NP_GUTTER_H) - NP_GUTTER_H)
#define NP_ROW_Y(row)     (NP_MARGIN_V + (row) * (NP_ROW_UNIT + NP_GUTTER_V))
#define NP_ROW_H(span)    ((span) * (NP_ROW_UNIT + NP_GUTTER_V) - NP_GUTTER_V)

/* Larguras usadas com frequencia, para nao repetir NP_COL_W literal. */
#define NP_W_FULL   NP_COL_W(12)   /* 896 */
#define NP_W_HALF   NP_COL_W(6)    /* 440 */
#define NP_W_MAIN   NP_COL_W(7)    /* 516 -- coluna de foco do arquetipo Hero */
#define NP_W_ASIDE  NP_COL_W(5)    /* 364 -- coluna de apoio do arquetipo Hero */
#define NP_X_ASIDE  NP_COL_X(7)

/* ------------------------------------------------------------------ */
/* Espaco -- escala fechada. Nada fora dela em codigo de tela.          */
/* ------------------------------------------------------------------ */

#define NP_SP_4  4
#define NP_SP_8  8
#define NP_SP_12 12
#define NP_SP_16 16
#define NP_SP_24 24
#define NP_SP_32 32
#define NP_SP_48 48

/* ------------------------------------------------------------------ */
/* Raio                                                                 */
/* ------------------------------------------------------------------ */

#define NP_RADIUS_CONTROL 8
#define NP_RADIUS_SURFACE 20

/* ------------------------------------------------------------------ */
/* Cor por papel semantico.                                             */
/*                                                                      */
/* Papel, nunca nome de cor: `np_c_positive()` e nao `np_c_green()`.    */
/* E o que permite trocar claro/escuro sem tocar em nenhuma tela.       */
/* ------------------------------------------------------------------ */

static inline lv_color_t np_c_bg(void)             { return lv_color_hex(0x0D0F18); }
static inline lv_color_t np_c_surface(void)        { return lv_color_hex(0x141721); }
static inline lv_color_t np_c_surface_raised(void) { return lv_color_hex(0x1B1E2D); }
static inline lv_color_t np_c_hairline(void)       { return lv_color_hex(0x1E2235); }

static inline lv_color_t np_c_text(void)           { return lv_color_hex(0xE8EAF2); }
static inline lv_color_t np_c_text_2(void)         { return lv_color_hex(0xBCC0CE); }
static inline lv_color_t np_c_text_3(void)         { return lv_color_hex(0x7A8298); }
static inline lv_color_t np_c_text_disabled(void)  { return lv_color_hex(0x464E64); }
static inline lv_color_t np_c_text_on_accent(void) { return lv_color_hex(0x090C12); }

static inline lv_color_t np_c_accent(void)         { return lv_color_hex(0xE8A83C); }
static inline lv_color_t np_c_accent_bg(void)      { return lv_color_hex(0x1C1900); }

static inline lv_color_t np_c_positive(void)       { return lv_color_hex(0x4ABB78); }
static inline lv_color_t np_c_positive_bg(void)    { return lv_color_hex(0x0F1D15); }
static inline lv_color_t np_c_negative(void)       { return lv_color_hex(0xD05252); }
static inline lv_color_t np_c_negative_bg(void)    { return lv_color_hex(0x241416); }

/* ------------------------------------------------------------------ */
/* Tipografia por papel.                                                */
/*                                                                      */
/* Este arquivo de referencia usa as Montserrat embutidas do LVGL para  */
/* compilar em qualquer lugar sem asset externo. No firmware, os        */
/* mesmos papeis apontam para o catalogo real (ui_tokens.hpp).          */
/* O catalogo subsetado por papel da SS5 do UI-LAYOUT-SYSTEM.md ainda   */
/* nao existe -- ver docs/STATUS.md.                                    */
/* ------------------------------------------------------------------ */

#define NP_FONT_HERO     (&lv_font_montserrat_48)  /* relogio da Home */
#define NP_FONT_DISPLAY  (&lv_font_montserrat_38)  /* valor de BTC, temperatura */
#define NP_FONT_TITLE    (&lv_font_montserrat_28)
#define NP_FONT_LG       (&lv_font_montserrat_22)
#define NP_FONT_MD       (&lv_font_montserrat_16)
#define NP_FONT_SM       (&lv_font_montserrat_14)

/* ------------------------------------------------------------------ */
/* Semantica de estado de dado -- usada por np_status_pill().           */
/* Offline-first: ausencia de dado e honesta, nunca tela de erro.       */
/* ------------------------------------------------------------------ */

typedef enum {
    NP_DATA_UNAVAILABLE = 0,
    NP_DATA_STALE       = 1,   /* veio do cache */
    NP_DATA_LIVE        = 2
} np_data_state_t;

static inline lv_color_t np_c_for_state(np_data_state_t state)
{
    switch (state) {
        case NP_DATA_LIVE:  return np_c_positive();
        case NP_DATA_STALE: return np_c_accent();
        default:            return np_c_text_3();
    }
}

static inline const char *np_label_for_state(np_data_state_t state)
{
    switch (state) {
        case NP_DATA_LIVE:  return "Ao vivo";
        case NP_DATA_STALE: return "Cache";
        default:            return "Indisponivel";
    }
}

#ifdef __cplusplus
}
#endif

#endif /* NP_TOKENS_H */
