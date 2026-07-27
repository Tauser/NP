/* ================================================================
 * SCREEN: Clima — LVGL v9.5
 *   • lv_button_create() instead of lv_btn_create()
 *   • lv_obj_set_style_flex_grow() instead of lv_obj_set_flex_grow()
 * ================================================================ */
#include "../novapanel.h"
#include <stdio.h>

void np_screen_clima(lv_obj_t *parent)
{
    lv_obj_t *scr = lv_obj_create(parent);
    lv_obj_set_size(scr, lv_pct(100), lv_pct(100));
    lv_obj_set_pos(scr, 0, 0);
    np_obj_clear_style(scr);
    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(scr,
        LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_STRETCH, LV_FLEX_ALIGN_STRETCH);
    lv_obj_set_style_pad_column(scr, 12, 0);
    np_screens[NP_SCR_CLIMA] = scr;

    /* ══ LEFT: current conditions ══ */
    lv_obj_t *left = np_card(scr);
    lv_obj_set_size(left, 220, lv_pct(100));
    lv_obj_set_style_pad_all(left, 18, 0);
    lv_obj_set_flex_flow(left, LV_FLEX_FLOW_COLUMN);

    np_label(left, "SÃO PAULO, SP", NP_F_XS, NP_C_TEXT_MUTED);

    lv_obj_t *big = lv_obj_create(left);
    np_obj_clear_style(big);
    lv_obj_set_size(big, lv_pct(100), 0);
    lv_obj_set_style_flex_grow(big, 1, 0);  /* v9 */
    lv_obj_set_flex_flow(big, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(big,
        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(big, 6, 0);

    np_label(big, "24°",             NP_F_HERO, NP_C_TEXT);
    np_label(big, "Parcial nublado", NP_F_SM,   NP_C_TEXT_DIM);

    lv_obj_t *mm = np_row(big);
    lv_obj_set_size(mm, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_pad_column(mm, 12, 0);
    np_label(mm, LV_SYMBOL_UP" 27°",   NP_F_SM, NP_C_BLUE);
    np_label(mm, LV_SYMBOL_DOWN" 18°", NP_F_SM, lv_color_hex(0x5A6478));

    np_label(big, "Sensação 26°", NP_F_XS, lv_color_hex(0x5A6478));

    lv_obj_t *sun_row = np_row(left);
    lv_obj_set_flex_align(sun_row,
        LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_border_side(sun_row, LV_BORDER_SIDE_TOP, 0);
    lv_obj_set_style_border_color(sun_row, NP_C_BORDER, 0);
    lv_obj_set_style_border_width(sun_row, 1, 0);
    lv_obj_set_style_pad_top(sun_row, 14, 0);

    static const struct { const char *sym; const char *time; const char *lbl; }
    suns[] = {
        { LV_SYMBOL_UP,   "06:18", "Nascer" },
        { LV_SYMBOL_DOWN, "18:43", "Pôr"    },
    };
    for (int i = 0; i < 2; i++) {
        lv_obj_t *sc = np_col(sun_row);
        lv_obj_set_size(sc, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        lv_obj_set_flex_align(sc,
            LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_row(sc, 3, 0);
        np_label(sc, suns[i].sym,  NP_F_LG, NP_C_ACCENT);
        np_label(sc, suns[i].time, NP_F_MD, NP_C_TEXT);
        np_label(sc, suns[i].lbl,  NP_F_XS, NP_C_TEXT_MUTED);
    }

    /* ══ CENTRE: forecasts ══ */
    lv_obj_t *mid = lv_obj_create(scr);
    np_obj_clear_style(mid);
    lv_obj_set_style_flex_grow(mid, 1, 0);  /* v9 */
    lv_obj_set_height(mid, lv_pct(100));
    lv_obj_set_flex_flow(mid, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(mid, 12, 0);

    /* hourly */
    lv_obj_t *hr = np_card(mid);
    lv_obj_set_size(hr, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(hr, 14, 0);
    np_label(hr, "PRÓXIMAS HORAS", NP_F_XS, NP_C_TEXT_MUTED);

    lv_obj_t *hr_row = np_row(hr);
    lv_obj_set_style_margin_top(hr_row, 12, 0);
    lv_obj_set_flex_align(hr_row,
        LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_END);

    static const struct { const char *h; const char *t; uint32_t tc; int precip; }
    hourly[] = {
        {"10h","25°",0xE8A83C,5 },{"11h","26°",0xE8A83C,10},
        {"12h","27°",0xE8A83C,3 },{"13h","26°",0xE8EAF2,15},
        {"14h","25°",0xE8EAF2,40},{"15h","23°",0x4F7ECB,60},
        {"16h","22°",0x4F7ECB,55},{"17h","21°",0x4F7ECB,20},
    };
    for (int i = 0; i < 8; i++) {
        lv_obj_t *hc = np_col(hr_row);
        lv_obj_set_size(hc, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        lv_obj_set_flex_align(hc,
            LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_row(hc, 5, 0);
        np_label(hc, hourly[i].h, NP_F_XS, lv_color_hex(0x5A6478));
        np_label(hc, hourly[i].t, NP_F_MD, lv_color_hex(hourly[i].tc));

        lv_obj_t *bar = lv_bar_create(hc);
        lv_obj_set_size(bar, 6, 32);
        lv_bar_set_range(bar, 0, 100);
        lv_bar_set_value(bar, hourly[i].precip, LV_ANIM_OFF);
        lv_obj_set_style_bg_color(bar, NP_C_BORDER, LV_PART_MAIN);
        lv_obj_set_style_bg_color(bar, NP_C_BLUE,   LV_PART_INDICATOR);
        lv_obj_set_style_radius(bar, 3, LV_PART_MAIN);
        lv_obj_set_style_radius(bar, 3, LV_PART_INDICATOR);

        char pct[8]; lv_snprintf(pct, sizeof(pct), "%d%%", hourly[i].precip);
        np_label(hc, pct, NP_F_XS, NP_C_BLUE);
    }

    /* 5-day */
    lv_obj_t *fd = np_card(mid);
    lv_obj_set_size(fd, lv_pct(100), 0);
    lv_obj_set_style_flex_grow(fd, 1, 0);  /* v9 */
    lv_obj_set_style_pad_all(fd, 14, 0);
    np_label(fd, "PRÓXIMOS 5 DIAS", NP_F_XS, NP_C_TEXT_MUTED);

    static const struct { const char *day; const char *cond; int prec; int mn; int mx; }
    days5[] = {
        {"Qua","Parcial nublado",10,18,27},
        {"Qui","Ensolarado",      5,17,29},
        {"Sex","Chuva fraca",    70,16,22},
        {"Sáb","Nublado",        45,15,21},
        {"Dom","Ensolarado",      5,16,28},
    };
    for (int i = 0; i < 5; i++) {
        lv_obj_t *dr = np_row(fd);
        lv_obj_set_style_border_side(dr, LV_BORDER_SIDE_BOTTOM, 0);
        lv_obj_set_style_border_color(dr, NP_C_SEP, 0);
        lv_obj_set_style_border_width(dr, 1, 0);
        lv_obj_set_style_pad_ver(dr, 7, 0);
        lv_obj_set_flex_align(dr,
            LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_column(dr, 10, 0);

        lv_obj_t *day_l = np_label(dr, days5[i].day, NP_F_SM, NP_C_TEXT_MED);
        lv_obj_set_width(day_l, 32);
        lv_obj_t *cond = np_label(dr, days5[i].cond, NP_F_XS,
                                  lv_color_hex(0x7A8298));
        lv_obj_set_style_flex_grow(cond, 1, 0);  /* v9 */

        char pp[8]; lv_snprintf(pp, sizeof(pp), "%d%%", days5[i].prec);
        np_label(dr, pp, NP_F_XS, lv_color_hex(0x5A6478));

        char mn[6], mx[6];
        lv_snprintf(mn, sizeof(mn), "%d°", days5[i].mn);
        lv_snprintf(mx, sizeof(mx), "%d°", days5[i].mx);
        np_label(dr, mn, NP_F_SM, NP_C_BLUE);
        np_label(dr, mx, NP_F_SM, NP_C_ACCENT);
    }

    /* ══ RIGHT: detail stats ══ */
    lv_obj_t *right = lv_obj_create(scr);
    np_obj_clear_style(right);
    lv_obj_set_size(right, 200, lv_pct(100));
    lv_obj_set_flex_flow(right, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(right, 10, 0);

    lv_obj_t *det = np_card(right);
    lv_obj_set_size(det, lv_pct(100), 0);
    lv_obj_set_style_flex_grow(det, 1, 0);  /* v9 */
    lv_obj_set_style_pad_all(det, 14, 0);
    np_label(det, "DETALHES", NP_F_XS, NP_C_TEXT_MUTED);

    static const struct { const char *k; const char *v; uint32_t vc; }
    details[] = {
        {"Vento",       "12 km/h NE", 0xE8EAF2},
        {"Umidade",     "54%",         0x4F7ECB},
        {"Pressão",     "1013 hPa",    0xE8EAF2},
        {"Ponto orv.",  "14°",         0xE8EAF2},
        {"Visibilidade","10 km",        0xE8EAF2},
        {"Nuvens",      "42%",          0x7A8298},
        {"Índice UV",   "5 — Mod.",    0xE8A83C},
        {"Precip. hoje","0 mm",         0x4F7ECB},
    };
    for (int i = 0; i < 8; i++) {
        lv_obj_t *dr = np_row(det);
        lv_obj_set_flex_align(dr,
            LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_margin_top(dr, 11, 0);
        np_label(dr, details[i].k, NP_F_XS, lv_color_hex(0x7A8298));
        np_label(dr, details[i].v, NP_F_SM, lv_color_hex(details[i].vc));
    }

    lv_obj_t *src = np_card(right);
    lv_obj_set_size(src, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(src, 12, 0);
    np_label(src, "FONTE",            NP_F_XS, NP_C_TEXT_MUTED);
    lv_obj_t *sl = np_label(src, "Open-Meteo · cache", NP_F_SM,
                            lv_color_hex(0x7A8298));
    lv_obj_set_style_margin_top(sl, 8, 0);
    np_label(src, "Atualizado 09:42", NP_F_XS, lv_color_hex(0x363C52));
}
