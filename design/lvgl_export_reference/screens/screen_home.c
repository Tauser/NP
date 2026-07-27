/* ================================================================
 * SCREEN: Home — LVGL v9.5
 *
 * v9 changes:
 *   • lv_button_create() instead of lv_btn_create()
 *   • lv_obj_set_style_flex_grow() instead of lv_obj_set_flex_grow()
 *   • lv_bar_set_start_value() removed (just lv_bar_set_value)
 *   • lv_obj_align() instead of lv_obj_center() where applicable
 * ================================================================ */
#include "../novapanel.h"

#define LEFT_W  362
#define COL_GAP  12

void np_screen_home(lv_obj_t *parent)
{
    lv_obj_t *scr = lv_obj_create(parent);
    lv_obj_set_size(scr, lv_pct(100), lv_pct(100));
    lv_obj_set_pos(scr, 0, 0);
    np_obj_clear_style(scr);
    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(scr,
        LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_STRETCH);
    lv_obj_set_style_pad_column(scr, COL_GAP, 0);
    np_screens[NP_SCR_HOME] = scr;

    /* ══ LEFT ══ */
    lv_obj_t *left = np_card(scr);
    lv_obj_set_size(left, LEFT_W, lv_pct(100));
    lv_obj_set_style_pad_all(left, 20, 0);
    lv_obj_set_flex_flow(left, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(left,
        LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_scrollbar_mode(left, LV_SCROLLBAR_MODE_OFF);

    /* clock row */
    lv_obj_t *clk_row = lv_obj_create(left);
    np_obj_clear_style(clk_row);
    lv_obj_set_size(clk_row, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(clk_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(clk_row,
        LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_END);
    lv_obj_set_style_pad_column(clk_row, 4, 0);

    np_label(clk_row, "09:42", NP_F_HERO, NP_C_TEXT);
    lv_obj_t *secs = np_label(clk_row, "30", NP_F_3XL, lv_color_hex(0x3A4252));
    lv_obj_set_style_pad_bottom(secs, 8, 0);

    lv_obj_t *date = np_label(left, "quarta-feira, 18 de junho",
                               NP_F_SM, NP_C_TEXT_DIM);
    lv_obj_set_style_margin_top(date, 8, 0);

    /* spacer */
    lv_obj_t *sp = lv_obj_create(left);
    np_obj_clear_style(sp);
    lv_obj_set_size(sp, 1, 1);
    lv_obj_set_style_flex_grow(sp, 1, 0);  /* v9 */

    lv_obj_t *hs = np_hsep(left);
    lv_obj_set_style_margin_bottom(hs, 14, 0);

    /* weather card */
    lv_obj_t *wx = lv_obj_create(left);
    lv_obj_set_size(wx, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(wx,     lv_color_hex(0x1B1E2D), 0);
    lv_obj_set_style_bg_opa(wx,       LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(wx, 0, 0);
    lv_obj_set_style_radius(wx,       10, 0);
    lv_obj_set_style_pad_all(wx,      14, 0);
    lv_obj_set_scrollbar_mode(wx,     LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_flex_flow(wx, LV_FLEX_FLOW_COLUMN);

    lv_obj_t *wr = np_row(wx);
    lv_obj_set_flex_align(wr,
        LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    np_label(wr, "24°", NP_F_4XL, NP_C_TEXT);

    lv_obj_t *city_col = np_col(wr);
    lv_obj_set_size(city_col, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_align(city_col,
        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_END);
    np_label(city_col, "São Paulo", NP_F_SM, lv_color_hex(0xC4C8D6));
    np_label(city_col, LV_SYMBOL_UP" 27°   "LV_SYMBOL_DOWN" 18°",
             NP_F_XS, NP_C_TEXT_MUTED);
    np_label(wx, "Parcial nublado", NP_F_XS, NP_C_TEXT_DIM);

    lv_obj_t *wsp = lv_obj_create(wx);
    lv_obj_set_size(wsp, lv_pct(100), 1);
    lv_obj_set_style_bg_color(wsp,     lv_color_hex(0x252A3C), 0);
    lv_obj_set_style_bg_opa(wsp,       LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(wsp, 0, 0);
    lv_obj_set_style_radius(wsp,       0, 0);
    lv_obj_set_style_margin_top(wsp,   11, 0);
    lv_obj_set_style_margin_bottom(wsp,11, 0);

    lv_obj_t *wst = np_row(wx);
    lv_obj_set_flex_align(wst,
        LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    static const struct { const char *l; const char *v; uint32_t c; } stats[] = {
        { "Vento",    "12 km/h", 0xE8EAF2 },
        { "Umidade",  "54%",     0xE8EAF2 },
        { "Sensação", "26°",     0xE8EAF2 },
        { "UV",       "Mod.",    0xE8A83C },
    };
    for (int i = 0; i < 4; i++) {
        lv_obj_t *sc = np_col(wst);
        lv_obj_set_size(sc, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        lv_obj_set_flex_align(sc,
            LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        np_label(sc, stats[i].l, NP_F_XS, NP_C_TEXT_MUTED);
        lv_obj_t *vl = np_label(sc, stats[i].v, NP_F_MD,
                                lv_color_hex(stats[i].c));
        lv_obj_set_style_margin_top(vl, 3, 0);
    }

    /* ══ RIGHT ══ */
    lv_obj_t *right = lv_obj_create(scr);
    lv_obj_set_style_flex_grow(right, 1, 0);   /* v9 */
    lv_obj_set_height(right, lv_pct(100));
    np_obj_clear_style(right);
    lv_obj_set_flex_flow(right, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(right,
        LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_row(right, COL_GAP, 0);

    /* Agenda mini card */
    lv_obj_t *ag = np_card(right);
    lv_obj_set_size(ag, lv_pct(100), 0);
    lv_obj_set_style_flex_grow(ag, 1, 0);      /* v9 */
    lv_obj_set_style_pad_all(ag, 16, 0);
    lv_obj_set_flex_flow(ag, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(ag,
        LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    lv_obj_t *agh = np_row(ag);
    lv_obj_set_flex_align(agh,
        LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(agh, 9, 0);
    lv_obj_set_style_margin_bottom(agh, 10, 0);
    np_label(agh, LV_SYMBOL_LIST, NP_F_SM, NP_C_ACCENT);
    lv_obj_t *ag_t = np_label(agh, "Agenda de hoje", NP_F_SM, NP_C_TEXT);
    lv_obj_set_style_flex_grow(ag_t, 1, 0);   /* v9 */
    np_label(agh, "4 eventos", NP_F_XS, NP_C_TEXT_MUTED);
    np_chip(agh, "Ver tudo >");

    static const struct { const char *t; const char *title; const char *sub; uint32_t c; }
    evts[] = {
        { "15:30", "Reunião — NoiseBot team", "Trabalho · 45 min", 0xE8A83C },
        { "17:00", "Academia",                "Pessoal · 1h",      0x4ABB78 },
        { "19:30", "Jantar com Marina",       "Pessoal · 2h",      0x4F7ECB },
        { "21:00", "Revisar firmware v1.4",   "Projeto · 30 min",  0xB77ABB },
    };
    for (int i = 0; i < 4; i++) {
        lv_obj_t *row = np_row(ag);
        lv_obj_set_style_pad_column(row, 11, 0);
        lv_obj_set_style_border_side(row, LV_BORDER_SIDE_BOTTOM, 0);
        lv_obj_set_style_border_color(row, NP_C_SEP, 0);
        lv_obj_set_style_border_width(row, 1, 0);
        lv_obj_set_style_pad_ver(row, 8, 0);

        lv_obj_t *tm = np_label(row, evts[i].t, NP_F_SM, NP_C_TEXT_MED);
        lv_obj_set_width(tm, 38);

        lv_obj_t *bar = lv_obj_create(row);
        lv_obj_set_size(bar, 3, 28);
        lv_obj_set_style_bg_color(bar,     lv_color_hex(evts[i].c), 0);
        lv_obj_set_style_bg_opa(bar,       LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(bar, 0, 0);
        lv_obj_set_style_radius(bar,       2, 0);

        lv_obj_t *info = np_col(row);
        lv_obj_set_style_flex_grow(info, 1, 0);  /* v9 */
        lv_obj_set_size(info, 0, LV_SIZE_CONTENT);
        lv_obj_t *ttl = np_label(info, evts[i].title, NP_F_SM, NP_C_TEXT);
        lv_label_set_long_mode(ttl, LV_LABEL_LONG_DOTS); /* v9: LONG_DOTS */
        lv_obj_set_width(ttl, 260);
        lv_obj_t *sub = np_label(info, evts[i].sub, NP_F_XS,
                                 lv_color_hex(0x5A6478));
        lv_obj_set_style_margin_top(sub, 1, 0);
    }

    /* Quick scenes */
    lv_obj_t *sc = np_card(right);
    lv_obj_set_size(sc, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(sc, 12, 0);
    lv_obj_set_style_pad_row(sc, 9, 0);
    lv_obj_set_flex_flow(sc, LV_FLEX_FLOW_COLUMN);

    np_label(sc, "CENAS RÁPIDAS", NP_F_XS, NP_C_TEXT_DARK);

    lv_obj_t *sc_row = np_row(sc);
    lv_obj_set_flex_align(sc_row,
        LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_STRETCH, LV_FLEX_ALIGN_STRETCH);
    lv_obj_set_style_pad_column(sc_row, 8, 0);

    static const struct { const char *name; const char *sym; uint32_t bg; uint32_t col; }
    scenes[] = {
        { "Bom dia", LV_SYMBOL_TINT,  0x1C1900, 0xE8A83C },
        { "Foco",    LV_SYMBOL_DRIVE, 0x121C2D, 0x4F7ECB },
    };
    for (int i = 0; i < 2; i++) {
        lv_obj_t *sb = lv_button_create(sc_row);  /* v9 */
        lv_obj_set_style_flex_grow(sb, 1, 0);     /* v9 */
        lv_obj_set_height(sb, 40);
        lv_obj_set_style_bg_color(sb,     lv_color_hex(scenes[i].bg), 0);
        lv_obj_set_style_border_color(sb, lv_color_hex(scenes[i].col), 0);
        lv_obj_set_style_border_width(sb, 1, 0);
        lv_obj_set_style_border_opa(sb,   LV_OPA_30, 0);
        lv_obj_set_style_radius(sb,       NP_R_BTN, 0);
        lv_obj_set_style_shadow_width(sb, 0, 0);
        lv_obj_set_flex_flow(sb, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(sb,
            LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_column(sb, 8, 0);
        np_label(sb, scenes[i].sym,  NP_F_MD, lv_color_hex(scenes[i].col));
        np_label(sb, scenes[i].name, NP_F_SM, NP_C_TEXT);
    }

    /* Bottom row: Player + Market */
    lv_obj_t *br = lv_obj_create(right);
    np_obj_clear_style(br);
    lv_obj_set_size(br, lv_pct(100), 0);
    lv_obj_set_style_flex_grow(br, 1, 0);   /* v9 */
    lv_obj_set_flex_flow(br, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(br,
        LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_STRETCH, LV_FLEX_ALIGN_STRETCH);
    lv_obj_set_style_pad_column(br, COL_GAP, 0);

    /* music player */
    lv_obj_t *pl = np_card(br);
    lv_obj_set_style_flex_grow(pl, 1, 0);  /* v9 */
    lv_obj_set_height(pl, lv_pct(100));
    lv_obj_set_style_pad_all(pl, 13, 0);
    lv_obj_set_flex_flow(pl, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(pl,
        LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    lv_obj_t *ptop = np_row(pl);
    lv_obj_set_style_pad_column(ptop, 10, 0);

    lv_obj_t *cov = lv_obj_create(ptop);
    lv_obj_set_size(cov, 34, 34);
    lv_obj_set_style_bg_color(cov,     NP_C_ACCENT, 0);
    lv_obj_set_style_radius(cov,       8, 0);
    lv_obj_set_style_border_width(cov, 0, 0);
    lv_obj_set_style_pad_all(cov,      0, 0);
    lv_obj_set_scrollbar_mode(cov, LV_SCROLLBAR_MODE_OFF);
    lv_obj_t *cov_ic = np_label(cov, LV_SYMBOL_AUDIO, NP_F_MD, NP_C_DARK_FG);
    lv_obj_align(cov_ic, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t *si = np_col(ptop);
    lv_obj_set_style_flex_grow(si, 1, 0);  /* v9 */
    lv_obj_set_size(si, 0, LV_SIZE_CONTENT);
    np_label(si, "Labirinto", NP_F_SM, NP_C_TEXT);
    np_label(si, "Tuyo · Pra Cima de Mim", NP_F_XS, lv_color_hex(0x5A6478));

    /* progress bar — v9: no lv_bar_set_start_value needed for simple bars */
    lv_obj_t *pb = lv_bar_create(pl);
    lv_obj_set_size(pb, lv_pct(100), 2);
    lv_bar_set_range(pb, 0, 100);
    lv_bar_set_value(pb, 17, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(pb, NP_C_BORDER, LV_PART_MAIN);
    lv_obj_set_style_bg_color(pb, NP_C_ACCENT,  LV_PART_INDICATOR);
    lv_obj_set_style_radius(pb, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(pb, 1, LV_PART_INDICATOR);

    /* playback controls */
    lv_obj_t *ctrl = np_row(pl);
    lv_obj_set_flex_align(ctrl,
        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(ctrl, 14, 0);
    np_label(ctrl, LV_SYMBOL_PREV, NP_F_MD, NP_C_TEXT_MUTED);

    lv_obj_t *play = lv_button_create(ctrl);  /* v9 */
    lv_obj_set_size(play, 30, 30);
    lv_obj_set_style_bg_color(play,     NP_C_ACCENT, 0);
    lv_obj_set_style_radius(play,       15, 0);
    lv_obj_set_style_shadow_width(play, 0, 0);
    lv_obj_t *play_ic = np_label(play, LV_SYMBOL_PAUSE, NP_F_XS, NP_C_DARK_FG);
    lv_obj_align(play_ic, LV_ALIGN_CENTER, 0, 0);

    np_label(ctrl, LV_SYMBOL_NEXT, NP_F_MD, NP_C_TEXT_MUTED);

    /* market mini */
    lv_obj_t *mk = np_card(br);
    lv_obj_set_style_flex_grow(mk, 1, 0);  /* v9 */
    lv_obj_set_height(mk, lv_pct(100));
    lv_obj_set_style_pad_all(mk, 13, 0);
    lv_obj_set_flex_flow(mk, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(mk,
        LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    lv_obj_t *mh = np_row(mk);
    lv_obj_set_style_pad_column(mh, 6, 0);
    lv_obj_set_style_margin_bottom(mh, 9, 0);
    np_label(mh, LV_SYMBOL_CHARGE, NP_F_SM, NP_C_ACCENT);
    np_label(mh, "Mercado", NP_F_SM, NP_C_TEXT);

    static const struct { const char *n; const char *p; const char *ch; uint32_t c; }
    mkt[] = {
        { "Dólar",    "R$ 5,42", "▼ 0,2%", 0xD05252 },
        { "Ibovespa", "138,5k",  "▼ 0,6%", 0xD05252 },
        { "Bitcoin",  "R$ 531k", "▲ 2,4%", 0x4ABB78 },
    };
    for (int i = 0; i < 3; i++) {
        lv_obj_t *ar = np_row(mk);
        lv_obj_set_flex_align(ar,
            LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_margin_bottom(ar, 6, 0);
        lv_obj_t *an = np_label(ar, mkt[i].n, NP_F_SM,
                                lv_color_hex(0x5A6478));
        lv_obj_set_style_flex_grow(an, 1, 0);  /* v9 */
        lv_obj_t *ap = np_label(ar, mkt[i].p, NP_F_SM, NP_C_TEXT);
        lv_obj_set_style_margin_right(ap, 6, 0);
        np_label(ar, mkt[i].ch, NP_F_XS, lv_color_hex(mkt[i].c));
    }
}
