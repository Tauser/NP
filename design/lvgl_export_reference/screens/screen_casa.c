/* ================================================================
 * SCREEN: Casa — LVGL v9.5
 *   • lv_button_create() instead of lv_btn_create()
 *   • lv_obj_set_style_flex_grow() instead of lv_obj_set_flex_grow()
 * ================================================================ */
#include "../novapanel.h"

void np_screen_casa(lv_obj_t *parent)
{
    lv_obj_t *scr = lv_obj_create(parent);
    lv_obj_set_size(scr, lv_pct(100), lv_pct(100));
    lv_obj_set_pos(scr, 0, 0);
    np_obj_clear_style(scr);
    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(scr,
        LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_row(scr, 12, 0);
    lv_obj_set_scrollbar_mode(scr, LV_SCROLLBAR_MODE_ON);
    np_screens[NP_SCR_CASA] = scr;

    /* CENAS */
    lv_obj_t *sc_hdr = np_row(scr);
    lv_obj_set_flex_align(sc_hdr,
        LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    np_label(sc_hdr, "CENAS", NP_F_XS, NP_C_TEXT_DARK);
    np_chip(sc_hdr, "Gerenciar cenas");

    lv_obj_t *sc_grid = lv_obj_create(scr);
    lv_obj_set_size(sc_grid, lv_pct(100), LV_SIZE_CONTENT);
    np_obj_clear_style(sc_grid);
    lv_obj_set_flex_flow(sc_grid, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(sc_grid,
        LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_STRETCH, LV_FLEX_ALIGN_STRETCH);
    lv_obj_set_style_pad_column(sc_grid, 9, 0);

    static const struct { const char *name; const char *sym; uint32_t col; uint32_t bg; }
    scenes[] = {
        { "Bom dia",   LV_SYMBOL_TINT,  0xE8A83C, 0x1C1900 },
        { "Foco",      LV_SYMBOL_DRIVE, 0x4F7ECB, 0x121C2D },
        { "Cinema",    LV_SYMBOL_IMAGE, 0x7B6CC8, 0x16122D },
        { "Boa noite", LV_SYMBOL_TINT,  0xB77ABB, 0x1C1527 },
    };
    for (int i = 0; i < 4; i++) {
        lv_obj_t *sb = lv_button_create(sc_grid);  /* v9 */
        lv_obj_set_style_flex_grow(sb, 1, 0);      /* v9 */
        lv_obj_set_height(sb, 48);
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

    /* DISPOSITIVOS */
    lv_obj_t *dv_hdr = np_row(scr);
    lv_obj_set_flex_align(dv_hdr,
        LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    np_label(dv_hdr, "DISPOSITIVOS", NP_F_XS, NP_C_TEXT_DARK);
    lv_obj_t *dv_btns = np_row(dv_hdr);
    lv_obj_set_size(dv_btns, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_pad_column(dv_btns, 7, 0);
    np_chip(dv_btns, "Gerenciar");
    np_chip(dv_btns, "+ Adicionar");

    lv_obj_t *dv_grid = lv_obj_create(scr);
    lv_obj_set_size(dv_grid, lv_pct(100), LV_SIZE_CONTENT);
    np_obj_clear_style(dv_grid);
    lv_obj_set_flex_flow(dv_grid, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(dv_grid, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_STRETCH, LV_FLEX_ALIGN_STRETCH);
    lv_obj_set_style_pad_column(dv_grid, 9, 0);

    static const struct { const char *room; const char *label; const char *val;
                          uint32_t valcol; bool on; }
    devs[] = {
        { "Sala",    "Luz sala",      "Ligada",    0x4ABB78, true  },
        { "Cozinha", "Luz cozinha",   "Desligada", 0x5A6478, false },
        { "Quarto",  "Luz quarto",    "Desligada", 0x5A6478, false },
        { "Sala",    "Tomada TV",     "Ligada",    0x4ABB78, true  },
        { "Cozinha", "Tomada cafet.", "Desligada", 0x5A6478, false },
        { "Sala",    "Ventilador",    "Desligada", 0x5A6478, false },
    };
    for (int i = 0; i < 6; i++) {
        lv_obj_t *dc = np_card(dv_grid);
        lv_obj_set_style_flex_grow(dc, 1, 0);  /* v9 */
        lv_obj_set_style_pad_all(dc, 13, 0);
        lv_obj_set_flex_flow(dc, LV_FLEX_FLOW_COLUMN);

        lv_obj_t *dh = np_row(dc);
        lv_obj_set_flex_align(dh,
            LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_margin_bottom(dh, 10, 0);
        np_label(dh, devs[i].room, NP_F_XS, lv_color_hex(0x5A6478));
        np_toggle(dh, devs[i].on);

        np_label(dc, devs[i].label, NP_F_SM, NP_C_TEXT);
        lv_obj_t *vl = np_label(dc, devs[i].val, NP_F_SM,
                                lv_color_hex(devs[i].valcol));
        lv_obj_set_style_margin_top(vl, 2, 0);
    }

    /* SENSORES */
    np_label(scr, "SENSORES AMBIENTAIS", NP_F_XS, NP_C_TEXT_DARK);

    lv_obj_t *sens = np_row(scr);
    lv_obj_set_flex_align(sens,
        LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_STRETCH, LV_FLEX_ALIGN_STRETCH);
    lv_obj_set_style_pad_column(sens, 9, 0);

    static const struct { const char *label; const char *val; const char *sub; uint32_t c; }
    snsr[] = {
        { "Temperatura", "24°C", "Sala",         0xE8A83C },
        { "Umidade",     "54%",  "Sala",          0x4F7ECB },
        { "CO₂",         "412",  "ppm — Normal",  0x4ABB78 },
        { "Luminosidade","1.240","lux — Boa",     0xE8EAF2 },
    };
    for (int i = 0; i < 4; i++) {
        lv_obj_t *sc2 = np_card(sens);
        lv_obj_set_style_flex_grow(sc2, 1, 0);  /* v9 */
        lv_obj_set_style_pad_all(sc2, 13, 0);
        np_label(sc2, snsr[i].label, NP_F_XS, lv_color_hex(0x5A6478));
        lv_obj_t *vl = np_label(sc2, snsr[i].val, NP_F_2XL,
                                lv_color_hex(snsr[i].c));
        lv_obj_set_style_margin_top(vl, 6, 0);
        lv_obj_t *sl = np_label(sc2, snsr[i].sub, NP_F_XS, NP_C_TEXT_MUTED);
        lv_obj_set_style_margin_top(sl, 2, 0);
    }
}
