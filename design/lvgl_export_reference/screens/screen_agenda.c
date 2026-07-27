/* ================================================================
 * SCREEN: Agenda — LVGL v9.5
 *
 * v9 changes:
 *   • lv_coord_t → int32_t in grid DSC arrays
 *   • lv_button_create() instead of lv_btn_create()
 *   • lv_obj_set_style_flex_grow() instead of lv_obj_set_flex_grow()
 *   • Left border: use lv_obj_set_style_border_side(LV_BORDER_SIDE_LEFT)
 *     (there are no separate lv_obj_set_style_border_left_* helpers in LVGL)
 *   • Dashed borders not natively supported; using solid border instead
 * ================================================================ */
#include "../novapanel.h"

void np_screen_agenda(lv_obj_t *parent)
{
    lv_obj_t *scr = lv_obj_create(parent);
    lv_obj_set_size(scr, lv_pct(100), lv_pct(100));
    lv_obj_set_pos(scr, 0, 0);
    np_obj_clear_style(scr);
    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(scr,
        LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_STRETCH, LV_FLEX_ALIGN_STRETCH);
    lv_obj_set_style_pad_column(scr, 12, 0);
    np_screens[NP_SCR_AGENDA] = scr;

    /* ══ LEFT ══ */
    lv_obj_t *left = lv_obj_create(scr);
    np_obj_clear_style(left);
    lv_obj_set_style_flex_grow(left, 135, 0);  /* v9 */
    lv_obj_set_height(left, lv_pct(100));
    lv_obj_set_flex_flow(left, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(left, 12, 0);

    /* next-event banner — left accent border via border_side */
    lv_obj_t *nxt = np_card(left);
    lv_obj_set_size(nxt, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_style_border_color(nxt, NP_C_ACCENT, 0);
    lv_obj_set_style_border_width(nxt, 3, 0);
    lv_obj_set_style_border_side(nxt, LV_BORDER_SIDE_LEFT, 0);  /* left only */
    lv_obj_set_style_pad_all(nxt, 14, 0);
    lv_obj_set_style_pad_left(nxt, 17, 0);   /* compensate wider border */

    np_label(nxt, "Próximo evento", NP_F_XS, NP_C_TEXT_MUTED);

    lv_obj_t *nx_row = np_row(nxt);
    lv_obj_set_style_pad_column(nx_row, 10, 0);
    lv_obj_set_style_margin_top(nx_row, 6, 0);
    np_label(nx_row, "15:30", NP_F_4XL, NP_C_ACCENT);
    np_label(nx_row, "Reunião — NoiseBot team", NP_F_MD, NP_C_TEXT);

    np_label(nxt, "Sala virtual · 45 min · em 1h 12min",
             NP_F_SM, lv_color_hex(0x5A6478));

    /* event list card */
    lv_obj_t *ec = np_card(left);
    lv_obj_set_size(ec, lv_pct(100), 0);
    lv_obj_set_style_flex_grow(ec, 1, 0);  /* v9 */
    lv_obj_set_style_pad_all(ec, 14, 0);
    lv_obj_set_flex_flow(ec, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(ec,
        LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    lv_obj_t *ech = np_row(ec);
    lv_obj_set_flex_align(ech,
        LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_margin_bottom(ech, 10, 0);
    np_label(ech, "quarta-feira, 18 de junho", NP_F_XS, NP_C_TEXT_MUTED);
    np_chip(ech, "+ Adicionar");

    static const struct { const char *t; const char *title; const char *sub; uint32_t c; }
    evts[] = {
        { "15:30", "Reunião — NoiseBot team", "Trabalho · 45 min", 0xE8A83C },
        { "17:00", "Academia",                "Pessoal · 1h",      0x4ABB78 },
        { "19:30", "Jantar com Marina",       "Pessoal · 2h",      0x4F7ECB },
        { "21:00", "Revisar firmware v1.4",   "Projeto · 30 min",  0xB77ABB },
    };
    for (int i = 0; i < 4; i++) {
        lv_obj_t *row = np_row(ec);
        lv_obj_set_style_border_side(row, LV_BORDER_SIDE_BOTTOM, 0);
        lv_obj_set_style_border_color(row, NP_C_SEP, 0);
        lv_obj_set_style_border_width(row, 1, 0);
        lv_obj_set_style_pad_column(row, 11, 0);
        lv_obj_set_style_pad_ver(row, 10, 0);

        lv_obj_t *tm = np_label(row, evts[i].t, NP_F_SM, NP_C_TEXT_MED);
        lv_obj_set_width(tm, 40);

        lv_obj_t *bar = lv_obj_create(row);
        lv_obj_set_size(bar, 3, 32);
        lv_obj_set_style_bg_color(bar,     lv_color_hex(evts[i].c), 0);
        lv_obj_set_style_bg_opa(bar,       LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(bar, 0, 0);
        lv_obj_set_style_radius(bar,       2, 0);

        lv_obj_t *info = np_col(row);
        lv_obj_set_style_flex_grow(info, 1, 0);  /* v9 */
        lv_obj_set_size(info, 0, LV_SIZE_CONTENT);
        np_label(info, evts[i].title, NP_F_SM, NP_C_TEXT);
        lv_obj_t *sub = np_label(info, evts[i].sub, NP_F_XS,
                                 lv_color_hex(0x5A6478));
        lv_obj_set_style_margin_top(sub, 2, 0);

        np_label(row, LV_SYMBOL_CLOSE, NP_F_XS, NP_C_TEXT_MUTED);
    }

    /* ══ RIGHT ══ */
    lv_obj_t *right = lv_obj_create(scr);
    np_obj_clear_style(right);
    lv_obj_set_style_flex_grow(right, 100, 0);  /* v9 */
    lv_obj_set_height(right, lv_pct(100));
    lv_obj_set_flex_flow(right, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(right, 12, 0);

    /* summary grid — v9: int32_t arrays (lv_coord_t is int32_t in v9) */
    lv_obj_t *sg = np_card(right);
    lv_obj_set_size(sg, lv_pct(100), LV_SIZE_CONTENT);
    np_label(sg, "Resumo", NP_F_XS, NP_C_TEXT_MUTED);

    static int32_t cols[] = { LV_PCT(50), LV_PCT(50), LV_GRID_TEMPLATE_LAST };
    static int32_t rows[] = { LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST };

    lv_obj_t *grid = lv_obj_create(sg);
    lv_obj_set_size(grid, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(grid,      LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(grid, 0, 0);
    lv_obj_set_style_pad_all(grid,     0, 0);
    lv_obj_set_style_margin_top(grid,  12, 0);
    lv_obj_set_style_pad_row(grid,     9, 0);
    lv_obj_set_style_pad_column(grid,  9, 0);
    lv_obj_set_grid_dsc_array(grid, cols, rows);
    lv_obj_set_scrollbar_mode(grid, LV_SCROLLBAR_MODE_OFF);

    static const struct { const char *val; const char *label; uint32_t col; } sums[] = {
        { "4",  "total",    0xE8EAF2 },
        { "1h", "próximo",  0xE8A83C },
        { "2",  "trabalho", 0xE8A83C },
        { "2",  "pessoal",  0x4ABB78 },
    };
    for (int i = 0; i < 4; i++) {
        lv_obj_t *cell = lv_obj_create(grid);
        lv_obj_set_grid_cell(cell, LV_GRID_ALIGN_STRETCH, i % 2, 1,
                             LV_GRID_ALIGN_STRETCH, i / 2, 1);
        lv_obj_set_style_bg_color(cell,     lv_color_hex(0x1B1E2D), 0);
        lv_obj_set_style_bg_opa(cell,       LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(cell, 0, 0);
        lv_obj_set_style_radius(cell,       9, 0);
        lv_obj_set_style_pad_all(cell,      12, 0);
        lv_obj_set_flex_flow(cell, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(cell,
            LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_scrollbar_mode(cell, LV_SCROLLBAR_MODE_OFF);
        np_label(cell, sums[i].val,   NP_F_4XL, lv_color_hex(sums[i].col));
        np_label(cell, sums[i].label, NP_F_XS,  lv_color_hex(0x5A6478));
    }

    /* tomorrow card — solid border instead of dashed (not supported natively) */
    lv_obj_t *tom = np_card(right);
    lv_obj_set_size(tom, lv_pct(100), 0);
    lv_obj_set_style_flex_grow(tom, 1, 0);  /* v9 */
    lv_obj_set_flex_flow(tom, LV_FLEX_FLOW_COLUMN);
    np_label(tom, "Amanhã", NP_F_XS, NP_C_TEXT_MUTED);

    lv_obj_t *empty = lv_obj_create(tom);
    lv_obj_set_size(empty, lv_pct(100), 0);
    lv_obj_set_style_flex_grow(empty, 1, 0);  /* v9 */
    lv_obj_set_style_bg_opa(empty,       LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_color(empty, lv_color_hex(0x252840), 0);
    lv_obj_set_style_border_width(empty, 1, 0);
    lv_obj_set_style_radius(empty,       9, 0);
    lv_obj_set_style_margin_top(empty,   12, 0);
    lv_obj_set_flex_flow(empty, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(empty,
        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(empty, 8, 0);
    lv_obj_set_scrollbar_mode(empty, LV_SCROLLBAR_MODE_OFF);
    np_label(empty, LV_SYMBOL_LIST,       NP_F_XL, lv_color_hex(0x363C52));
    np_label(empty, "Sem compromissos",   NP_F_SM,  NP_C_TEXT_MUTED);
}
