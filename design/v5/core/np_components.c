#include "np_components.h"
#include "np_styles.h"

#include <string.h>

/* ---------------- escrita guardada (R7) ---------------- */

void np_set_text(lv_obj_t *label, const char *text)
{
    if (label == NULL || text == NULL) {
        return;
    }
    if (lv_strcmp(lv_label_get_text(label), text) != 0) {
        lv_label_set_text(label, text);
    }
}

void np_set_text_color(lv_obj_t *obj, lv_color_t color)
{
    if (obj == NULL) {
        return;
    }
    if (!lv_color_eq(lv_obj_get_style_text_color(obj, LV_PART_MAIN), color)) {
        lv_obj_set_style_text_color(obj, color, 0);
    }
}

void np_set_bg_color(lv_obj_t *obj, lv_color_t color)
{
    if (obj == NULL) {
        return;
    }
    if (!lv_color_eq(lv_obj_get_style_bg_color(obj, LV_PART_MAIN), color)) {
        lv_obj_set_style_bg_color(obj, color, 0);
    }
}

/* ---------------- primitivas ---------------- */

static lv_obj_t *bare_box(lv_obj_t *parent, int32_t x, int32_t y, int32_t w, int32_t h)
{
    lv_obj_t *obj = lv_obj_create(parent);
    lv_obj_remove_style_all(obj);
    lv_obj_set_pos(obj, x, y);
    lv_obj_set_size(obj, w, h);
    lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    return obj;
}

lv_obj_t *np_scene(lv_obj_t *parent)
{
    lv_obj_t *obj = lv_obj_create(parent);
    lv_obj_remove_style_all(obj);
    lv_obj_add_style(obj, np_st_screen(), 0);
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, LV_PCT(100), LV_PCT(100));
    lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    return obj;
}

lv_obj_t *np_group(lv_obj_t *parent, int32_t x, int32_t y, int32_t w, int32_t h)
{
    return bare_box(parent, x, y, w, h);
}

lv_obj_t *np_surface(lv_obj_t *parent, int32_t x, int32_t y, int32_t w, int32_t h)
{
    lv_obj_t *obj = bare_box(parent, x, y, w, h);
    lv_obj_add_style(obj, np_st_surface(), 0);
    return obj;
}

lv_obj_t *np_raised(lv_obj_t *parent, int32_t x, int32_t y, int32_t w, int32_t h)
{
    lv_obj_t *obj = bare_box(parent, x, y, w, h);
    lv_obj_add_style(obj, np_st_raised(), 0);
    return obj;
}

lv_obj_t *np_fill(lv_obj_t *parent, int32_t x, int32_t y, int32_t w, int32_t h,
                  lv_color_t color, lv_opa_t opa, int32_t radius)
{
    lv_obj_t *obj = bare_box(parent, x, y, w, h);
    lv_obj_set_style_bg_color(obj, color, 0);
    lv_obj_set_style_bg_opa(obj, opa, 0);
    lv_obj_set_style_radius(obj, radius, 0);
    return obj;
}

lv_obj_t *np_label(lv_obj_t *parent, const char *text, const lv_font_t *font,
                   lv_color_t color, int32_t x, int32_t y, int32_t w,
                   lv_text_align_t align)
{
    lv_obj_t *obj = lv_label_create(parent);
    lv_obj_add_style(obj, np_st_label(), 0);
    lv_obj_set_style_text_font(obj, font, 0);
    lv_obj_set_style_text_color(obj, color, 0);
    lv_obj_set_style_text_align(obj, align, 0);
    lv_label_set_long_mode(obj, LV_LABEL_LONG_MODE_CLIP);
    lv_obj_set_pos(obj, x, y);
    lv_obj_set_width(obj, w);
    lv_label_set_text(obj, text != NULL ? text : "");
    return obj;
}

lv_obj_t *np_hline(lv_obj_t *parent, int32_t x, int32_t y, int32_t w)
{
    lv_obj_t *obj = bare_box(parent, x, y, w, 1);
    lv_obj_add_style(obj, np_st_hairline(), 0);
    return obj;
}

lv_obj_t *np_vline(lv_obj_t *parent, int32_t x, int32_t y, int32_t h)
{
    lv_obj_t *obj = bare_box(parent, x, y, 1, h);
    lv_obj_add_style(obj, np_st_hairline(), 0);
    return obj;
}

lv_obj_t *np_dot(lv_obj_t *parent, int32_t x, int32_t y, int32_t size, lv_color_t color)
{
    return np_fill(parent, x, y, size, size, color, LV_OPA_COVER, LV_RADIUS_CIRCLE);
}

/* ---------------- compostos ---------------- */

np_metric_t np_metric(lv_obj_t *parent, int32_t x, int32_t y, int32_t w,
                      const char *caption, const char *value, const lv_font_t *value_font)
{
    np_metric_t m;
    m.caption = np_label(parent, caption, NP_FONT_SM, np_c_text_3(), x, y, w, LV_TEXT_ALIGN_LEFT);
    m.value   = np_label(parent, value, value_font, np_c_text(), x, y + NP_SP_24, w, LV_TEXT_ALIGN_LEFT);
    return m;
}

np_pill_t np_status_pill(lv_obj_t *parent, int32_t x, int32_t y, np_data_state_t state)
{
    np_pill_t pill;
    const lv_color_t color = np_c_for_state(state);
    pill.dot   = np_dot(parent, x, y + 5, 8, color);
    pill.label = np_label(parent, np_label_for_state(state), NP_FONT_SM, color,
                          x + 8 + NP_SP_8, y, 140, LV_TEXT_ALIGN_LEFT);
    return pill;
}

void np_status_pill_set(np_pill_t *pill, np_data_state_t state)
{
    const lv_color_t color = np_c_for_state(state);
    np_set_bg_color(pill->dot, color);
    np_set_text(pill->label, np_label_for_state(state));
    np_set_text_color(pill->label, color);
}

np_clock_t np_clock(lv_obj_t *parent, int32_t x, int32_t y, const lv_font_t *font,
                    lv_color_t color)
{
    np_clock_t clock;
    /* Largura por slot derivada da altura de linha da fonte: evita
     * LV_SIZE_CONTENT (R2) sem precisar medir glifo em tempo de build. */
    const int32_t digit_w = font->line_height * 3 / 5;
    const int32_t colon_w = digit_w / 2;
    const int32_t width[5] = { digit_w, digit_w, colon_w, digit_w, digit_w };
    int32_t cursor = x;

    for (int i = 0; i < 5; i++) {
        clock.slot[i] = np_label(parent, i == 2 ? ":" : "-", font,
                                 i == 2 ? np_c_accent() : color,
                                 cursor, y, width[i], LV_TEXT_ALIGN_CENTER);
        cursor += width[i];
    }
    return clock;
}

void np_clock_set(np_clock_t *clock, const char *hhmm)
{
    char one[2] = { 0, 0 };

    if (hhmm == NULL || strlen(hhmm) != 5) {
        return;
    }
    for (int i = 0; i < 5; i++) {
        one[0] = hhmm[i];
        np_set_text(clock->slot[i], one);
    }
}

lv_obj_t *np_button(lv_obj_t *parent, int32_t x, int32_t y, int32_t w, int32_t h,
                    const char *text, bool primary)
{
    lv_obj_t *obj = lv_button_create(parent);
    lv_obj_remove_style_all(obj);
    lv_obj_set_pos(obj, x, y);
    lv_obj_set_size(obj, w, h);
    lv_obj_set_style_radius(obj, NP_RADIUS_CONTROL, 0);
    lv_obj_set_style_shadow_width(obj, 0, 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(obj, primary ? np_c_accent() : np_c_surface(), 0);
    lv_obj_set_style_border_width(obj, primary ? 0 : 1, 0);
    lv_obj_set_style_border_color(obj, np_c_hairline(), 0);

    lv_obj_t *text_label = lv_label_create(obj);
    lv_obj_set_style_text_font(text_label, NP_FONT_MD, 0);
    lv_obj_set_style_text_color(text_label, primary ? np_c_text_on_accent() : np_c_text(), 0);
    lv_label_set_text(text_label, text != NULL ? text : "");
    lv_obj_center(text_label);
    return obj;
}

np_row_t np_row(lv_obj_t *parent, int32_t x, int32_t y, int32_t w, int32_t h)
{
    np_row_t row;
    row.root = bare_box(parent, x, y, w, h);
    lv_obj_add_style(row.root, np_st_row(), 0);

    row.accent = np_fill(row.root, 0, 0, 3, h, np_c_accent(), LV_OPA_COVER, 0);
    row.lead   = np_label(row.root, "", NP_FONT_LG, np_c_accent(), NP_SP_16, NP_SP_12, 96,
                          LV_TEXT_ALIGN_LEFT);
    row.title  = np_label(row.root, "", NP_FONT_MD, np_c_text(), 128, NP_SP_12, w - 128 - NP_SP_16,
                          LV_TEXT_ALIGN_LEFT);
    row.meta   = np_label(row.root, "", NP_FONT_SM, np_c_text_3(), 128, h - NP_SP_24 - NP_SP_4,
                          w - 128 - NP_SP_16, LV_TEXT_ALIGN_LEFT);
    return row;
}

void np_row_set(np_row_t *row, const char *lead, const char *title, const char *meta,
                lv_color_t accent_color, bool visible)
{
    if (!visible) {
        lv_obj_add_flag(row->root, LV_OBJ_FLAG_HIDDEN);
        return;
    }
    lv_obj_remove_flag(row->root, LV_OBJ_FLAG_HIDDEN);
    np_set_text(row->lead, lead);
    np_set_text_color(row->lead, accent_color);
    np_set_text(row->title, title);
    np_set_text(row->meta, meta);
    np_set_bg_color(row->accent, accent_color);
}

np_tile_t np_tile(lv_obj_t *parent, int32_t x, int32_t y, int32_t w, int32_t h,
                  const char *icon, const char *title, const char *state)
{
    np_tile_t tile;
    tile.root = bare_box(parent, x, y, w, h);
    lv_obj_add_style(tile.root, np_st_tile(), 0);

    tile.icon  = np_label(tile.root, icon, NP_FONT_TITLE, np_c_text_2(), 0, 0, w - 2 * NP_SP_16,
                          LV_TEXT_ALIGN_LEFT);
    tile.title = np_label(tile.root, title, NP_FONT_MD, np_c_text(), 0, h - 2 * NP_SP_16 - 44,
                          w - 2 * NP_SP_16, LV_TEXT_ALIGN_LEFT);
    tile.state = np_label(tile.root, state, NP_FONT_SM, np_c_text_3(), 0, h - 2 * NP_SP_16 - 20,
                          w - 2 * NP_SP_16, LV_TEXT_ALIGN_LEFT);
    return tile;
}

void np_tile_set_on(np_tile_t *tile, bool on, const char *state_text)
{
    lv_obj_remove_style(tile->root, on ? np_st_tile() : np_st_tile_on(), 0);
    lv_obj_add_style(tile->root, on ? np_st_tile_on() : np_st_tile(), 0);
    np_set_text_color(tile->icon, on ? np_c_accent() : np_c_text_2());
    np_set_text(tile->state, state_text);
    np_set_text_color(tile->state, on ? np_c_accent() : np_c_text_3());
}

np_spark_t np_spark(lv_obj_t *parent, int32_t x, int32_t y, int32_t w, int32_t h)
{
    np_spark_t spark;
    memset(&spark, 0, sizeof(spark));
    spark.line = lv_line_create(parent);
    lv_obj_set_pos(spark.line, x, y);
    lv_obj_set_size(spark.line, w, h);
    lv_obj_set_style_line_width(spark.line, 2, 0);
    lv_obj_set_style_line_rounded(spark.line, true, 0);
    lv_obj_set_style_line_color(spark.line, np_c_text_3(), 0);
    lv_obj_add_flag(spark.line, LV_OBJ_FLAG_HIDDEN);
    return spark;
}

void np_spark_set(np_spark_t *spark, const int32_t *samples, uint8_t count,
                  int32_t w, int32_t h, lv_color_t color)
{
    /* Menos de 2 amostras nao e uma serie: esconder e honesto, interpolar
     * seria inventar dado (UX_LAYOUT_REDESIGN.md SS2). */
    if (samples == NULL || count < 2) {
        lv_obj_add_flag(spark->line, LV_OBJ_FLAG_HIDDEN);
        return;
    }
    if (count > 24) {
        count = 24;
    }

    int32_t lo = samples[0];
    int32_t hi = samples[0];
    for (uint8_t i = 1; i < count; i++) {
        if (samples[i] < lo) lo = samples[i];
        if (samples[i] > hi) hi = samples[i];
    }
    const int32_t span = (hi - lo) > 0 ? (hi - lo) : 1;

    for (uint8_t i = 0; i < count; i++) {
        spark->pts[i].x = (int32_t)i * (w - 1) / (count - 1);
        spark->pts[i].y = (h - 1) - ((samples[i] - lo) * (h - 1) / span);
    }
    lv_line_set_points(spark->line, spark->pts, count);
    lv_obj_set_style_line_color(spark->line, color, 0);
    lv_obj_remove_flag(spark->line, LV_OBJ_FLAG_HIDDEN);
}

np_segbar_t np_segbar(lv_obj_t *parent, int32_t x, int32_t y, int32_t w, uint8_t count)
{
    np_segbar_t bar;
    memset(&bar, 0, sizeof(bar));
    if (count > 6) {
        count = 6;
    }
    bar.count = count;

    const int32_t gap = NP_SP_8;
    const int32_t seg_w = (w - gap * (count - 1)) / count;
    for (uint8_t i = 0; i < count; i++) {
        bar.seg[i] = np_fill(parent, x + i * (seg_w + gap), y, seg_w, 4,
                             np_c_hairline(), LV_OPA_COVER, 2);
    }
    return bar;
}

void np_segbar_set(np_segbar_t *bar, uint8_t done, lv_color_t color)
{
    for (uint8_t i = 0; i < bar->count; i++) {
        np_set_bg_color(bar->seg[i], i < done ? color : np_c_hairline());
    }
}

lv_obj_t *np_dots(lv_obj_t *parent, int32_t y, uint8_t count, uint8_t active)
{
    const int32_t dot = 6;
    const int32_t wide = 20;
    const int32_t gap = NP_SP_8;
    int32_t total = 0;

    for (uint8_t i = 0; i < count; i++) {
        total += (i == active ? wide : dot) + (i + 1 < count ? gap : 0);
    }

    lv_obj_t *root = np_group(parent, (NP_SCREEN_W - total) / 2, y, total, dot);
    int32_t cursor = 0;
    for (uint8_t i = 0; i < count; i++) {
        const int32_t w = (i == active ? wide : dot);
        np_fill(root, cursor, 0, w, dot, i == active ? np_c_accent() : np_c_hairline(),
                LV_OPA_COVER, dot / 2);
        cursor += w + gap;
    }
    return root;
}
