/* ================================================================
 * SCREEN: Timer (Pomodoro) — LVGL v9.5
 *
 * v9 changes:
 *   • lv_button_create() instead of lv_btn_create()
 *   • lv_obj_set_style_flex_grow() instead of lv_obj_set_flex_grow()
 *   • lv_obj_set_style_arc_rounded() split into:
 *       lv_obj_set_style_arc_round_start(arc, true, LV_PART_INDICATOR)
 *       lv_obj_set_style_arc_round_end(arc, true, LV_PART_INDICATOR)
 * ================================================================ */
#include "../novapanel.h"
#include <stdio.h>

static lv_obj_t *g_arc   = NULL;
static lv_obj_t *g_label = NULL;
static lv_obj_t *g_btn   = NULL;

static void preset_cb(lv_event_t *e)
{
    int32_t sec = (int32_t)(uintptr_t)lv_event_get_user_data(e);
    np_state.timer_sec   = sec;
    np_state.timer_total = sec;
    np_state.timer_running = false;
    if (g_arc)   lv_arc_set_value(g_arc, 100);
    if (g_label) {
        char buf[8];
        lv_snprintf(buf, sizeof(buf), "%02d:%02d",
                    (int)(sec / 60), (int)(sec % 60));
        lv_label_set_text(g_label, buf);
    }
    if (g_btn) lv_label_set_text(lv_obj_get_child(g_btn, 0), "Iniciar");
}

static void toggle_cb(lv_event_t *e)
{
    np_state.timer_running = !np_state.timer_running;
    if (g_btn)
        lv_label_set_text(lv_obj_get_child(g_btn, 0),
                          np_state.timer_running ? "Pausar" : "Iniciar");
}

static void reset_cb(lv_event_t *e)
{
    np_state.timer_running = false;
    np_state.timer_sec = np_state.timer_total;
    if (g_arc) lv_arc_set_value(g_arc, 100);
    if (g_label) {
        char buf[8];
        lv_snprintf(buf, sizeof(buf), "%02d:%02d",
                    (int)(np_state.timer_total / 60),
                    (int)(np_state.timer_total % 60));
        lv_label_set_text(g_label, buf);
    }
    if (g_btn) lv_label_set_text(lv_obj_get_child(g_btn, 0), "Iniciar");
}

void np_screen_timer(lv_obj_t *parent)
{
    lv_obj_t *scr = lv_obj_create(parent);
    lv_obj_set_size(scr, lv_pct(100), lv_pct(100));
    lv_obj_set_pos(scr, 0, 0);
    np_obj_clear_style(scr);
    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(scr,
        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(scr, 22, 0);
    np_screens[NP_SCR_TIMER] = scr;

    /* ── Arc ring ── */
    lv_obj_t *arc_wrap = lv_obj_create(scr);
    lv_obj_set_size(arc_wrap, 200, 200);
    np_obj_clear_style(arc_wrap);

    lv_obj_t *arc = lv_arc_create(arc_wrap);
    lv_obj_set_size(arc, 200, 200);
    lv_obj_align(arc, LV_ALIGN_CENTER, 0, 0);
    lv_arc_set_rotation(arc, 270);
    lv_arc_set_bg_angles(arc, 0, 360);
    lv_arc_set_range(arc, 0, 100);
    lv_arc_set_value(arc, 100);
    lv_obj_remove_style(arc, NULL, LV_PART_KNOB);
    lv_obj_clear_flag(arc, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_set_style_arc_color(arc, NP_C_BORDER, LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc, 10,          LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc, NP_C_ACCENT, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(arc, 10,          LV_PART_INDICATOR);

    /* v9: arc_rounded split into round_start + round_end */
    lv_obj_set_style_arc_round_start(arc, true, LV_PART_INDICATOR);
    lv_obj_set_style_arc_round_end(arc,   true, LV_PART_INDICATOR);

    g_arc = arc;

    /* centred label */
    lv_obj_t *inner = lv_obj_create(arc_wrap);
    lv_obj_set_size(inner, 160, 80);
    lv_obj_align(inner, LV_ALIGN_CENTER, 0, 0);
    np_obj_clear_style(inner);
    lv_obj_set_flex_flow(inner, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(inner,
        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    g_label = np_label(inner, "25:00", NP_F_HERO, NP_C_TEXT);
    np_label(inner, "Pomodoro", NP_F_XS, lv_color_hex(0x5A6478));

    /* ── Preset chips ── */
    lv_obj_t *presets = lv_obj_create(scr);
    np_obj_clear_style(presets);
    lv_obj_set_size(presets, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(presets, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(presets,
        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(presets, 8, 0);

    static const struct { const char *label; int32_t sec; } chips[] = {
        { "15 min",  900  },
        { "25 min",  1500 },
        { "45 min",  2700 },
        { "1h",      3600 },
    };
    for (int i = 0; i < 4; i++) {
        lv_obj_t *ch = lv_button_create(presets);  /* v9 */
        lv_obj_set_height(ch, 36);
        lv_obj_set_width(ch, LV_SIZE_CONTENT);
        bool active = (chips[i].sec == np_state.timer_total);
        lv_obj_set_style_bg_color(ch,     active ? NP_C_ACCENT_BG : NP_C_CARD, 0);
        lv_obj_set_style_border_color(ch, active ? NP_C_ACCENT_BD : NP_C_BORDER, 0);
        lv_obj_set_style_border_width(ch, 1, 0);
        lv_obj_set_style_radius(ch,       NP_R_BTN, 0);
        lv_obj_set_style_shadow_width(ch, 0, 0);
        lv_obj_set_style_pad_hor(ch, 16, 0);
        lv_obj_t *cl = np_label(ch, chips[i].label, NP_F_SM,
                                active ? NP_C_ACCENT : NP_C_TEXT_DIM);
        lv_obj_align(cl, LV_ALIGN_CENTER, 0, 0);
        lv_obj_add_event_cb(ch, preset_cb, LV_EVENT_CLICKED,
                            (void *)(uintptr_t)chips[i].sec);
    }

    /* ── Controls ── */
    lv_obj_t *ctrl = lv_obj_create(scr);
    np_obj_clear_style(ctrl);
    lv_obj_set_size(ctrl, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(ctrl, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(ctrl,
        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(ctrl, 10, 0);

    g_btn = lv_button_create(ctrl);  /* v9 */
    lv_obj_set_size(g_btn, LV_SIZE_CONTENT, 48);
    lv_obj_set_style_bg_color(g_btn,     NP_C_ACCENT, 0);
    lv_obj_set_style_radius(g_btn,       NP_R_BTN, 0);
    lv_obj_set_style_shadow_width(g_btn, 0, 0);
    lv_obj_set_style_pad_hor(g_btn, 32, 0);
    lv_obj_t *btn_lbl = np_label(g_btn, "Iniciar", NP_F_MD, NP_C_DARK_FG);
    lv_obj_align(btn_lbl, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(g_btn, toggle_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *rst = lv_button_create(ctrl);  /* v9 */
    lv_obj_set_size(rst, 48, 48);
    lv_obj_set_style_bg_color(rst,     NP_C_CARD, 0);
    lv_obj_set_style_border_color(rst, NP_C_BORDER, 0);
    lv_obj_set_style_border_width(rst, 1, 0);
    lv_obj_set_style_radius(rst,       NP_R_BTN, 0);
    lv_obj_set_style_shadow_width(rst, 0, 0);
    lv_obj_t *rst_ic = np_label(rst, LV_SYMBOL_REFRESH, NP_F_LG, NP_C_TEXT_DIM);
    lv_obj_align(rst_ic, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(rst, reset_cb, LV_EVENT_CLICKED, NULL);
}
