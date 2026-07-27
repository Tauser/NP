/* ================================================================
 * NOVAPANEL — LVGL Theme  (updated for 7" display legibility)
 * Target : 1024 × 600 px  |  LVGL v9.5  |  ESP32-P4
 * ================================================================ */
#ifndef NOVAPANEL_THEME_H
#define NOVAPANEL_THEME_H

#include "lvgl.h"

/* ── Colors ───────────────────────────────────────────────────── */
#define NP_C_BG           lv_color_hex(0x0D0F18)
#define NP_C_CARD         lv_color_hex(0x141721)
#define NP_C_CARD2        lv_color_hex(0x1B1E2D)
#define NP_C_BORDER       lv_color_hex(0x1E2235)
#define NP_C_SEP          lv_color_hex(0x181B28)
#define NP_C_ACCENT       lv_color_hex(0xE8A83C)
#define NP_C_ACCENT_BG    lv_color_hex(0x1C1900)
#define NP_C_ACCENT_BD    lv_color_hex(0x2C2700)
#define NP_C_TEXT         lv_color_hex(0xE8EAF2)
#define NP_C_TEXT_DIM     lv_color_hex(0x7A8298)
#define NP_C_TEXT_MUTED   lv_color_hex(0x464E64)
#define NP_C_TEXT_DARK    lv_color_hex(0x30364A)
#define NP_C_TEXT_MED     lv_color_hex(0xBCC0CE)
#define NP_C_GREEN        lv_color_hex(0x4ABB78)
#define NP_C_GREEN_BG     lv_color_hex(0x0F1D15)
#define NP_C_GREEN_BD     lv_color_hex(0x1A3828)
#define NP_C_BLUE         lv_color_hex(0x4F7ECB)
#define NP_C_RED          lv_color_hex(0xD05252)
#define NP_C_RED_BG       lv_color_hex(0x3A1818)
#define NP_C_RED_BD       lv_color_hex(0x5A2020)
#define NP_C_PURPLE       lv_color_hex(0xB77ABB)
#define NP_C_RAIL_BG      lv_color_hex(0x0B0D16)
#define NP_C_ITEM_BG      lv_color_hex(0x141721)
#define NP_C_ITEM_BG2     lv_color_hex(0x252A3C)
#define NP_C_DARK_FG      lv_color_hex(0x090C12)

/* ── Dimensions ───────────────────────────────────────────────── */
#define NP_W              1024
#define NP_H              600
#define NP_RAIL_W         62
#define NP_TOPBAR_H       60       /* bumped 54→60 for 7" touch comfort */
#define NP_CONTENT_W      (NP_W - NP_RAIL_W)
#define NP_CONTENT_H      (NP_H - NP_TOPBAR_H)
#define NP_PAD            14
#define NP_GAP            12
#define NP_R_CARD         12
#define NP_R_BTN          9
#define NP_R_SM           7
#define NP_BDW            1
#define NP_DOTS_H         16       /* screen-indicator dots strip height */

/* ── Fonts — scaled for 7" legibility ────────────────────────── *
 * Enable in lv_conf.h:
 *   #define LV_FONT_MONTSERRAT_12 1   ← minimum readable on 7"
 *   #define LV_FONT_MONTSERRAT_14 1
 *   #define LV_FONT_MONTSERRAT_16 1
 *   #define LV_FONT_MONTSERRAT_20 1
 *   #define LV_FONT_MONTSERRAT_24 1
 *   #define LV_FONT_MONTSERRAT_28 1
 *   #define LV_FONT_MONTSERRAT_32 1
 *   #define LV_FONT_MONTSERRAT_48 1
 *
 * NP_F_XS bumped 10→12, NP_F_SM bumped 12→14 vs original spec.
 * ---------------------------------------------------------------- */
#define NP_F_XS    (&lv_font_montserrat_12)   /* labels, secondary */
#define NP_F_SM    (&lv_font_montserrat_14)   /* body small        */
#define NP_F_MD    (&lv_font_montserrat_14)   /* body              */
#define NP_F_LG    (&lv_font_montserrat_16)   /* body large        */
#define NP_F_XL    (&lv_font_montserrat_20)   /* subheading        */
#define NP_F_2XL   (&lv_font_montserrat_24)   /* heading           */
#define NP_F_3XL   (&lv_font_montserrat_28)   /* display small     */
#define NP_F_4XL   (&lv_font_montserrat_32)   /* display           */
#define NP_F_HERO  (&lv_font_montserrat_48)   /* hero / clock      */

/* ── Helper: transparent container ───────────────────────────── */
static inline void np_obj_clear_style(lv_obj_t *obj)
{
    lv_obj_set_style_bg_opa(obj,       LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_pad_all(obj,      0, 0);
    lv_obj_set_style_radius(obj,       0, 0);
    lv_obj_set_scrollbar_mode(obj,     LV_SCROLLBAR_MODE_OFF);
}

/* ── Helper: standard card ───────────────────────────────────── */
static inline lv_obj_t *np_card(lv_obj_t *parent)
{
    lv_obj_t *o = lv_obj_create(parent);
    lv_obj_set_style_bg_color(o,     NP_C_CARD, 0);
    lv_obj_set_style_bg_opa(o,       LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(o, NP_C_BORDER, 0);
    lv_obj_set_style_border_width(o, NP_BDW, 0);
    lv_obj_set_style_radius(o,       NP_R_CARD, 0);
    lv_obj_set_style_pad_all(o,      16, 0);
    lv_obj_set_scrollbar_mode(o,     LV_SCROLLBAR_MODE_OFF);
    return o;
}

/* ── Helper: label ───────────────────────────────────────────── */
static inline lv_obj_t *np_label(lv_obj_t *parent,
                                  const char *text,
                                  const lv_font_t *font,
                                  lv_color_t color)
{
    lv_obj_t *l = lv_label_create(parent);
    lv_label_set_text(l, text);
    lv_obj_set_style_text_font(l,  font,  0);
    lv_obj_set_style_text_color(l, color, 0);
    return l;
}

/* ── Helper: toggle switch ───────────────────────────────────── */
static inline lv_obj_t *np_toggle(lv_obj_t *parent, bool checked)
{
    lv_obj_t *sw = lv_switch_create(parent);
    lv_obj_set_size(sw, 44, 24);
    lv_obj_set_style_bg_color(sw, NP_C_ITEM_BG2,        LV_PART_MAIN);
    lv_obj_set_style_border_color(sw, NP_C_BORDER,       LV_PART_MAIN);
    lv_obj_set_style_border_width(sw, 1,                 LV_PART_MAIN);
    lv_obj_set_style_bg_color(sw, lv_color_hex(0x2C3820),
                              LV_PART_INDICATOR | LV_STATE_CHECKED);
    lv_obj_set_style_bg_color(sw, NP_C_TEXT_DIM,         LV_PART_KNOB);
    lv_obj_set_style_bg_color(sw, NP_C_ACCENT,
                              LV_PART_KNOB | LV_STATE_CHECKED);
    lv_obj_set_style_pad_all(sw, 2, LV_PART_KNOB);
    if (checked) lv_obj_add_state(sw, LV_STATE_CHECKED);
    return sw;
}

/* ── Helper: horizontal separator ────────────────────────────── */
static inline lv_obj_t *np_hsep(lv_obj_t *parent)
{
    lv_obj_t *s = lv_obj_create(parent);
    lv_obj_set_size(s, lv_pct(100), 1);
    lv_obj_set_style_bg_color(s,     NP_C_BORDER, 0);
    lv_obj_set_style_bg_opa(s,       LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s, 0, 0);
    lv_obj_set_style_pad_all(s,      0, 0);
    lv_obj_set_style_radius(s,       0, 0);
    return s;
}

/* ── Helper: row flex container ──────────────────────────────── */
static inline lv_obj_t *np_row(lv_obj_t *parent)
{
    lv_obj_t *o = lv_obj_create(parent);
    np_obj_clear_style(o);
    lv_obj_set_size(o, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(o, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(o, LV_FLEX_ALIGN_START,
                           LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    return o;
}

/* ── Helper: column flex container ───────────────────────────── */
static inline lv_obj_t *np_col(lv_obj_t *parent)
{
    lv_obj_t *o = lv_obj_create(parent);
    np_obj_clear_style(o);
    lv_obj_set_size(o, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(o, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(o, LV_FLEX_ALIGN_START,
                           LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    return o;
}

/* ── Helper: accent chip button ─────────────────────────────── */
static inline lv_obj_t *np_chip(lv_obj_t *parent, const char *text)
{
    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_set_size(btn, LV_SIZE_CONTENT, 30);
    lv_obj_set_style_bg_color(btn,     NP_C_ACCENT_BG, 0);
    lv_obj_set_style_border_color(btn, NP_C_ACCENT_BD, 0);
    lv_obj_set_style_border_width(btn, 1, 0);
    lv_obj_set_style_radius(btn,       15, 0);
    lv_obj_set_style_pad_hor(btn,      14, 0);
    lv_obj_set_style_shadow_width(btn, 0, 0);
    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_font(lbl,  NP_F_SM,     0);
    lv_obj_set_style_text_color(lbl, NP_C_ACCENT, 0);
    lv_obj_align(lbl, LV_ALIGN_CENTER, 0, 0);
    return btn;
}

/* ── Helper: icon button (40×40) ─────────────────────────────── */
static inline lv_obj_t *np_icon_btn(lv_obj_t *parent, const char *symbol)
{
    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_set_size(btn, 40, 40);
    lv_obj_set_style_bg_color(btn,     NP_C_ITEM_BG, 0);
    lv_obj_set_style_border_color(btn, NP_C_BORDER,  0);
    lv_obj_set_style_border_width(btn, 1, 0);
    lv_obj_set_style_radius(btn,       NP_R_BTN, 0);
    lv_obj_set_style_shadow_width(btn, 0, 0);
    lv_obj_t *ic = lv_label_create(btn);
    lv_label_set_text(ic, symbol);
    lv_obj_set_style_text_color(ic, NP_C_TEXT_DIM, 0);
    lv_obj_align(ic, LV_ALIGN_CENTER, 0, 0);
    return btn;
}

#endif /* NOVAPANEL_THEME_H */
