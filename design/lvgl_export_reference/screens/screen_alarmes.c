/* ================================================================
 * SCREEN: Alarmes — LVGL v9.5
 *   • lv_button_create() instead of lv_btn_create()
 *   • lv_obj_set_style_flex_grow() instead of lv_obj_set_flex_grow()
 * ================================================================ */
#include "../novapanel.h"

static void alarm_toggle_cb(lv_event_t *e)
{
    int idx = (int)(uintptr_t)lv_event_get_user_data(e);
    np_state.alarm_on[idx] = !np_state.alarm_on[idx];
}

void np_screen_alarmes(lv_obj_t *parent)
{
    lv_obj_t *scr = lv_obj_create(parent);
    lv_obj_set_size(scr, lv_pct(100), lv_pct(100));
    lv_obj_set_pos(scr, 0, 0);
    np_obj_clear_style(scr);
    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(scr,
        LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_STRETCH, LV_FLEX_ALIGN_STRETCH);
    lv_obj_set_style_pad_column(scr, 12, 0);
    np_screens[NP_SCR_ALARMES] = scr;

    /* ══ LEFT ══ */
    lv_obj_t *left = lv_obj_create(scr);
    np_obj_clear_style(left);
    lv_obj_set_style_flex_grow(left, 140, 0);  /* v9 */
    lv_obj_set_height(left, lv_pct(100));
    lv_obj_set_flex_flow(left, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(left, 9, 0);
    lv_obj_set_scrollbar_mode(left, LV_SCROLLBAR_MODE_ON);

    static const struct {
        const char *time; const char *label; const char *days; bool on;
    } alarms[] = {
        { "07:00", "Acordar",         "Seg–Sex",       true  },
        { "09:15", "Reunião matinal", "Seg, Qua, Sex", true  },
        { "12:30", "Almoço",          "Seg–Sex",       false },
        { "22:30", "Dormir",          "Diário",        true  },
    };
    for (int i = 0; i < 4; i++) {
        lv_obj_t *card = np_card(left);
        lv_obj_set_size(card, lv_pct(100), LV_SIZE_CONTENT);
        lv_obj_set_style_pad_all(card, 16, 0);

        lv_obj_t *row = np_row(card);
        lv_obj_set_flex_align(row,
            LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_column(row, 14, 0);

        lv_obj_t *info = np_col(row);
        lv_obj_set_style_flex_grow(info, 1, 0);  /* v9 */
        lv_obj_set_size(info, 0, LV_SIZE_CONTENT);

        lv_obj_t *tr = np_row(info);
        lv_obj_set_style_pad_column(tr, 10, 0);
        lv_obj_set_flex_align(tr,
            LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_BASELINE, LV_FLEX_ALIGN_BASELINE);

        lv_color_t tcol = alarms[i].on ? NP_C_TEXT : NP_C_TEXT_DIM;
        np_label(tr, alarms[i].time,  NP_F_4XL, tcol);
        np_label(tr, alarms[i].label, NP_F_MD,
                 alarms[i].on ? NP_C_TEXT : NP_C_TEXT_MUTED);
        np_label(info, alarms[i].days, NP_F_SM, lv_color_hex(0x5A6478));

        lv_obj_t *sw = np_toggle(row, alarms[i].on);
        lv_obj_add_event_cb(sw, alarm_toggle_cb, LV_EVENT_VALUE_CHANGED,
                            (void *)(uintptr_t)i);

        lv_obj_t *del = lv_button_create(row);  /* v9 */
        lv_obj_set_size(del, 32, 32);
        lv_obj_set_style_bg_color(del,     lv_color_hex(0x1A1010), 0);
        lv_obj_set_style_border_color(del, lv_color_hex(0x2A1818), 0);
        lv_obj_set_style_border_width(del, 1, 0);
        lv_obj_set_style_radius(del,       NP_R_SM, 0);
        lv_obj_set_style_shadow_width(del, 0, 0);
        lv_obj_t *del_ic = np_label(del, LV_SYMBOL_TRASH, NP_F_XS, NP_C_RED);
        lv_obj_align(del_ic, LV_ALIGN_CENTER, 0, 0);
    }

    /* ══ RIGHT ══ */
    lv_obj_t *right = lv_obj_create(scr);
    np_obj_clear_style(right);
    lv_obj_set_style_flex_grow(right, 100, 0);  /* v9 */
    lv_obj_set_height(right, lv_pct(100));
    lv_obj_set_flex_flow(right, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(right, 12, 0);

    /* next alarm */
    lv_obj_t *nxt = np_card(right);
    lv_obj_set_size(nxt, lv_pct(100), 0);
    lv_obj_set_style_flex_grow(nxt, 1, 0);  /* v9 */
    lv_obj_set_flex_flow(nxt, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(nxt,
        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    np_label(nxt, "Próximo alarme ativo", NP_F_XS, NP_C_TEXT_MUTED);

    lv_obj_t *na_row = np_row(nxt);
    lv_obj_set_style_pad_column(na_row, 14, 0);
    lv_obj_set_style_margin_top(na_row, 14, 0);

    lv_obj_t *bell_box = lv_obj_create(na_row);
    lv_obj_set_size(bell_box, 44, 44);
    lv_obj_set_style_bg_color(bell_box,     NP_C_ACCENT_BG, 0);
    lv_obj_set_style_border_color(bell_box, NP_C_ACCENT_BD, 0);
    lv_obj_set_style_border_width(bell_box, 1, 0);
    lv_obj_set_style_radius(bell_box,       10, 0);
    lv_obj_set_style_pad_all(bell_box,      0, 0);
    lv_obj_set_scrollbar_mode(bell_box, LV_SCROLLBAR_MODE_OFF);
    lv_obj_t *bell_ic = np_label(bell_box, LV_SYMBOL_BELL, NP_F_XL, NP_C_ACCENT);
    lv_obj_align(bell_ic, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t *na_info = np_col(na_row);
    lv_obj_set_size(na_info, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    np_label(na_info, "07:00", NP_F_HERO, NP_C_ACCENT);
    lv_obj_t *na_lbl = np_label(na_info, "Acordar", NP_F_MD, NP_C_TEXT);
    lv_obj_set_style_margin_top(na_lbl, 4, 0);

    lv_obj_t *na_days = np_label(nxt, "Seg–Sex", NP_F_SM, lv_color_hex(0x5A6478));
    lv_obj_set_style_margin_top(na_days, 10, 0);

    /* add alarm card */
    lv_obj_t *add = np_card(right);
    lv_obj_set_size(add, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(add, 16, 0);
    lv_obj_set_flex_flow(add, LV_FLEX_FLOW_COLUMN);
    np_label(add, "Novo alarme", NP_F_XS, NP_C_TEXT_MUTED);

    lv_obj_t *picker_row = np_row(add);
    lv_obj_set_style_pad_column(picker_row, 8, 0);
    lv_obj_set_style_margin_top(picker_row, 12, 0);
    lv_obj_set_style_margin_bottom(picker_row, 10, 0);

    static const struct { const char *label; const char *val; }
    pickers[] = { { "Horário", "07:00" }, { "Repetir", "Seg–Sex" } };
    for (int i = 0; i < 2; i++) {
        lv_obj_t *pk = lv_obj_create(picker_row);
        lv_obj_set_style_flex_grow(pk, 1, 0);  /* v9 */
        lv_obj_set_height(pk, LV_SIZE_CONTENT);
        lv_obj_set_style_bg_color(pk,     lv_color_hex(0x1B1E2D), 0);
        lv_obj_set_style_bg_opa(pk,       LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(pk, NP_C_BORDER, 0);
        lv_obj_set_style_border_width(pk, 1, 0);
        lv_obj_set_style_radius(pk,       9, 0);
        lv_obj_set_style_pad_all(pk,      10, 0);
        lv_obj_set_flex_flow(pk, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(pk,
            LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_scrollbar_mode(pk, LV_SCROLLBAR_MODE_OFF);
        np_label(pk, pickers[i].label, NP_F_XS, lv_color_hex(0x5A6478));
        np_label(pk, pickers[i].val,   NP_F_LG, NP_C_TEXT);
    }

    lv_obj_t *add_btn = lv_button_create(add);  /* v9 */
    lv_obj_set_size(add_btn, lv_pct(100), 44);
    lv_obj_set_style_bg_color(add_btn,     NP_C_ACCENT_BG, 0);
    lv_obj_set_style_border_color(add_btn, NP_C_ACCENT_BD, 0);
    lv_obj_set_style_border_width(add_btn, 1, 0);
    lv_obj_set_style_radius(add_btn,       NP_R_BTN, 0);
    lv_obj_set_style_shadow_width(add_btn, 0, 0);
    lv_obj_set_flex_flow(add_btn, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(add_btn,
        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(add_btn, 8, 0);
    np_label(add_btn, LV_SYMBOL_PLUS,        NP_F_SM, NP_C_ACCENT);
    np_label(add_btn, "Adicionar alarme",    NP_F_SM, NP_C_ACCENT);
}
