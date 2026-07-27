/* ================================================================
 * SCREEN: Mercado — LVGL v9.5
 *
 * v9 draw API changes (the biggest change in this file):
 *   • lv_draw_ctx_t *  →  lv_layer_t *
 *   • lv_event_get_draw_ctx(e)  →  lv_event_get_layer(e)
 *   • lv_draw_line() points now embedded in dsc (p1/p2 as lv_point_precise_t)
 *     old: lv_draw_line(ctx, &dsc, &p1, &p2)
 *     new: dsc.p1 = p1; dsc.p2 = p2; lv_draw_line(layer, &dsc);
 *   • lv_draw_rect() still (layer, &dsc, &area) but with lv_layer_t *
 *   • lv_button_create() instead of lv_btn_create()
 *   • lv_obj_set_style_flex_grow() instead of lv_obj_set_flex_grow()
 * ================================================================ */
#include "../novapanel.h"
#include <stdio.h>

/* ── Candle data ─────────────────────────────────────────────── */
static const struct { int32_t o; int32_t c; int32_t h; int32_t l; }
candles[] = {
    {97200,97800,98300,96900},{97800,97300,98100,97000},
    {97300,98600,99100,97200},{98600,98100,99000,97900},
    {98100,99200,99500,98000},{99200,98800,99600,98500},
    {98800,100100,100400,98700},{100100,99600,100500,99400},
    {99600,100900,101200,99500},{100900,100300,101300,100200},
    {100300,101800,102000,100100},{101800,103100,103400,101700},
    {103100,102500,103800,102300},{102500,103842,104521,100841},
};
#define CANDLE_N  (sizeof(candles)/sizeof(candles[0]))

/* ── v9.5 draw event ─────────────────────────────────────────── */
static void draw_candles(lv_event_t *e)
{
    /* v9: lv_event_get_layer() replaces lv_event_get_draw_ctx() */
    lv_layer_t *layer = lv_event_get_layer(e);

    lv_obj_t *obj = lv_event_get_target(e);
    lv_area_t obj_area;
    lv_obj_get_coords(obj, &obj_area);

    int32_t w     = lv_area_get_width(&obj_area);
    int32_t h     = lv_area_get_height(&obj_area);
    int32_t lo    = 96000, hi = 105000;
    int32_t range = hi - lo;

    int32_t cw  = w / (int32_t)CANDLE_N;
    int32_t gap = cw / 4;
    int32_t bw  = cw - gap * 2;
    if (bw < 2) bw = 2;

    for (int i = 0; i < (int)CANDLE_N; i++) {
        bool      bull = candles[i].c >= candles[i].o;
        lv_color_t col = bull ? NP_C_GREEN : NP_C_RED;

        int32_t x0 = obj_area.x1 + i * cw + gap;
        int32_t cx = x0 + bw / 2;

        int32_t yH = obj_area.y1 + h - (int32_t)((int64_t)(candles[i].h - lo) * h / range);
        int32_t yL = obj_area.y1 + h - (int32_t)((int64_t)(candles[i].l - lo) * h / range);
        int32_t yO = obj_area.y1 + h - (int32_t)((int64_t)(candles[i].o - lo) * h / range);
        int32_t yC = obj_area.y1 + h - (int32_t)((int64_t)(candles[i].c - lo) * h / range);

        /* ── Wick ── v9: points embedded in dsc, call lv_draw_line(layer, &dsc) */
        lv_draw_line_dsc_t wick;
        lv_draw_line_dsc_init(&wick);
        wick.color = col;
        wick.width = 1;
        wick.opa   = LV_OPA_COVER;
        /* v9: lv_point_precise_t (supports sub-pixel; use int coords) */
        wick.p1 = (lv_point_precise_t){ .x = cx, .y = yH };
        wick.p2 = (lv_point_precise_t){ .x = cx, .y = yL };
        lv_draw_line(layer, &wick);

        /* ── Body ── lv_draw_rect(layer, &dsc, &area) — same sig, new first arg */
        lv_draw_rect_dsc_t body;
        lv_draw_rect_dsc_init(&body);
        body.bg_color = col;
        body.bg_opa   = LV_OPA_COVER;
        body.radius   = 1;

        lv_area_t ba = {
            .x1 = x0,
            .y1 = LV_MIN(yO, yC),
            .x2 = x0 + bw,
            .y2 = LV_MAX(yO, yC),
        };
        if (ba.y1 == ba.y2) ba.y2 = ba.y1 + 1;
        lv_draw_rect(layer, &body, &ba);
    }
}

/* ================================================================ */
void np_screen_mercado(lv_obj_t *parent)
{
    lv_obj_t *scr = lv_obj_create(parent);
    lv_obj_set_size(scr, lv_pct(100), lv_pct(100));
    lv_obj_set_pos(scr, 0, 0);
    np_obj_clear_style(scr);
    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(scr,
        LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_STRETCH, LV_FLEX_ALIGN_STRETCH);
    lv_obj_set_style_pad_column(scr, 12, 0);
    np_screens[NP_SCR_MERCADO] = scr;

    /* ══ LEFT: chart panel ══ */
    lv_obj_t *left = np_card(scr);
    lv_obj_set_style_flex_grow(left, 1, 0);  /* v9 */
    lv_obj_set_height(left, lv_pct(100));
    lv_obj_set_style_pad_all(left, 16, 0);
    lv_obj_set_flex_flow(left, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(left,
        LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    /* header row */
    lv_obj_t *lh = np_row(left);
    lv_obj_set_flex_align(lh,
        LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(lh, 10, 0);
    lv_obj_set_style_margin_bottom(lh, 12, 0);

    lv_obj_t *btc_badge = lv_obj_create(lh);
    lv_obj_set_size(btc_badge, 24, 24);
    lv_obj_set_style_bg_color(btc_badge,     lv_color_hex(0xF7931A), 0);
    lv_obj_set_style_radius(btc_badge,       12, 0);
    lv_obj_set_style_border_width(btc_badge, 0, 0);
    lv_obj_set_style_pad_all(btc_badge,      0, 0);
    lv_obj_set_scrollbar_mode(btc_badge, LV_SCROLLBAR_MODE_OFF);
    lv_obj_t *bl = np_label(btc_badge, "₿", NP_F_SM, NP_C_DARK_FG);
    lv_obj_align(bl, LV_ALIGN_CENTER, 0, 0);

    np_label(lh, "Bitcoin · BTC/USD", NP_F_SM, NP_C_TEXT);

    lv_obj_t *lh_sp = lv_obj_create(lh);
    np_obj_clear_style(lh_sp);
    lv_obj_set_style_flex_grow(lh_sp, 1, 0);  /* v9 */

    /* period selector */
    lv_obj_t *per = lv_obj_create(lh);
    lv_obj_set_size(per, LV_SIZE_CONTENT, 30);
    lv_obj_set_style_bg_color(per,     lv_color_hex(0x1B1E2D), 0);
    lv_obj_set_style_bg_opa(per,       LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(per, NP_C_BORDER, 0);
    lv_obj_set_style_border_width(per, 1, 0);
    lv_obj_set_style_radius(per,       8, 0);
    lv_obj_set_style_pad_all(per,      3, 0);
    lv_obj_set_flex_flow(per, LV_FLEX_FLOW_ROW);
    lv_obj_set_scrollbar_mode(per, LV_SCROLLBAR_MODE_OFF);

    static const char *periods[] = { "1h", "1D", "1S", "1M" };
    for (int i = 0; i < 4; i++) {
        lv_obj_t *pb = lv_obj_create(per);
        lv_obj_set_size(pb, LV_SIZE_CONTENT, lv_pct(100));
        lv_obj_set_style_bg_color(pb,     i == 1 ? lv_color_hex(0x252A3C)
                                                  : lv_color_hex(0x1B1E2D), 0);
        lv_obj_set_style_bg_opa(pb,       LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(pb, 0, 0);
        lv_obj_set_style_radius(pb,       6, 0);
        lv_obj_set_style_pad_hor(pb,      10, 0);
        lv_obj_set_style_pad_ver(pb,      0, 0);
        lv_obj_set_scrollbar_mode(pb, LV_SCROLLBAR_MODE_OFF);
        lv_obj_t *pl = np_label(pb, periods[i], NP_F_XS,
                                i == 1 ? NP_C_TEXT : NP_C_TEXT_MUTED);
        lv_obj_align(pl, LV_ALIGN_CENTER, 0, 0);
    }

    /* price row */
    lv_obj_t *pr = np_row(left);
    lv_obj_set_flex_align(pr,
        LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(pr, 20, 0);
    lv_obj_set_style_border_side(pr, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_color(pr, NP_C_BORDER, 0);
    lv_obj_set_style_border_width(pr, 1, 0);
    lv_obj_set_style_pad_bottom(pr, 12, 0);
    lv_obj_set_style_margin_bottom(pr, 12, 0);

    lv_obj_t *pinfo = np_col(pr);
    lv_obj_set_size(pinfo, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    np_label(pinfo, "103.842", NP_F_HERO, NP_C_TEXT);
    np_label(pinfo, "USD", NP_F_XS, lv_color_hex(0x5A6478));
    np_label(pr, "▲ +2,41%", NP_F_LG, NP_C_GREEN);

    lv_obj_t *pr_sp = lv_obj_create(pr);
    np_obj_clear_style(pr_sp);
    lv_obj_set_style_flex_grow(pr_sp, 1, 0);  /* v9 */

    /* OHLC */
    lv_obj_t *ohlc = np_row(pr);
    lv_obj_set_size(ohlc, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_pad_column(ohlc, 18, 0);

    static const struct { const char *label; const char *val; uint32_t c; }
    ohlc_items[] = {
        { "Abertura",  "101.398", 0xE8EAF2 },
        { "Máx",       "104.521", 0x4ABB78 },
        { "Mín",       "100.841", 0xD05252 },
        { "Volume 24h","28,4 bi", 0xE8EAF2 },
    };
    for (int i = 0; i < 4; i++) {
        lv_obj_t *oc = np_col(ohlc);
        lv_obj_set_size(oc, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        lv_obj_set_flex_align(oc,
            LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_row(oc, 3, 0);
        np_label(oc, ohlc_items[i].label, NP_F_XS, NP_C_TEXT_MUTED);
        np_label(oc, ohlc_items[i].val,   NP_F_SM, lv_color_hex(ohlc_items[i].c));
    }

    /* Candlestick canvas — uses LV_EVENT_DRAW_MAIN_END + lv_event_get_layer() */
    lv_obj_t *chart_area = lv_obj_create(left);
    lv_obj_set_style_flex_grow(chart_area, 1, 0);  /* v9 */
    lv_obj_set_size(chart_area, lv_pct(100), 0);
    np_obj_clear_style(chart_area);
    lv_obj_add_event_cb(chart_area, draw_candles, LV_EVENT_DRAW_MAIN_END, NULL);

    /* ══ RIGHT: sidebar ══ */
    lv_obj_t *right = lv_obj_create(scr);
    np_obj_clear_style(right);
    lv_obj_set_size(right, 256, lv_pct(100));
    lv_obj_set_flex_flow(right, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(right, 12, 0);

    /* USD/BRL */
    lv_obj_t *usd = np_card(right);
    lv_obj_set_size(usd, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(usd, 14, 0);
    np_label(usd, "DÓLAR · USD/BRL", NP_F_XS, NP_C_TEXT_MUTED);
    lv_obj_t *ul = np_label(usd, "R$ 5,42", NP_F_4XL, NP_C_TEXT);
    lv_obj_set_style_margin_top(ul, 8, 0);
    np_label(usd, "▼ 0,18% · 24h", NP_F_SM, NP_C_RED);
    np_hsep(usd);
    lv_obj_t *ud1 = np_row(usd);
    lv_obj_set_flex_align(ud1,
        LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_margin_top(ud1, 10, 0);
    np_label(ud1, "Máx 24h", NP_F_SM, lv_color_hex(0x5A6478));
    np_label(ud1, "R$ 5,48", NP_F_SM, NP_C_TEXT);
    lv_obj_t *ud2 = np_row(usd);
    lv_obj_set_flex_align(ud2,
        LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_margin_top(ud2, 4, 0);
    np_label(ud2, "Mín 24h", NP_F_SM, lv_color_hex(0x5A6478));
    np_label(ud2, "R$ 5,38", NP_F_SM, NP_C_TEXT);

    /* Ibovespa */
    lv_obj_t *ibov = np_card(right);
    lv_obj_set_size(ibov, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(ibov, 14, 0);
    np_label(ibov, "IBOVESPA", NP_F_XS, NP_C_TEXT_MUTED);
    lv_obj_t *il = np_label(ibov, "138.540", NP_F_3XL, NP_C_TEXT);
    lv_obj_set_style_margin_top(il, 8, 0);
    np_label(ibov, "▼ 0,60% · 24h", NP_F_SM, NP_C_RED);

    /* Meta / favorites */
    lv_obj_t *meta = np_card(right);
    lv_obj_set_size(meta, lv_pct(100), 0);
    lv_obj_set_style_flex_grow(meta, 1, 0);  /* v9 */
    lv_obj_set_style_pad_all(meta, 14, 0);
    lv_obj_set_flex_flow(meta, LV_FLEX_FLOW_COLUMN);

    np_label(meta, "FONTE DOS DADOS", NP_F_XS, NP_C_TEXT_MUTED);

    static const struct { const char *k; const char *v; bool green; }
    meta_rows[] = {
        { "API",        "CoinGecko", false },
        { "Atualizado", "09:42",     false },
        { "Estado",     "Online",    true  },
    };
    for (int i = 0; i < 3; i++) {
        lv_obj_t *mr = np_row(meta);
        lv_obj_set_flex_align(mr,
            LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_margin_top(mr, 7, 0);
        np_label(mr, meta_rows[i].k, NP_F_SM, lv_color_hex(0x5A6478));
        np_label(mr, meta_rows[i].v, NP_F_SM,
                 meta_rows[i].green ? NP_C_GREEN : NP_C_TEXT);
    }

    np_hsep(meta);
    np_label(meta, "FAVORITOS", NP_F_XS, NP_C_TEXT_MUTED);

    lv_obj_t *fav = lv_obj_create(meta);
    lv_obj_set_size(fav, lv_pct(100), 0);
    lv_obj_set_style_flex_grow(fav, 1, 0);  /* v9 */
    lv_obj_set_style_bg_opa(fav,       LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_color(fav, lv_color_hex(0x2C2700), 0);
    lv_obj_set_style_border_width(fav, 1, 0);
    lv_obj_set_style_radius(fav,       9, 0);
    lv_obj_set_style_margin_top(fav,   9, 0);
    lv_obj_set_flex_flow(fav, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(fav,
        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(fav, 7, 0);
    lv_obj_set_scrollbar_mode(fav, LV_SCROLLBAR_MODE_OFF);
    np_label(fav, LV_SYMBOL_PLUS,       NP_F_LG, NP_C_ACCENT);
    np_label(fav, "Adicionar favorito", NP_F_SM, NP_C_ACCENT);
}
