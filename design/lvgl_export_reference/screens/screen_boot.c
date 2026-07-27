/* ================================================================
 * SCREEN: Boot — LVGL v9.5
 *
 * Full-screen splash shown at power-on.
 * Call np_boot_advance(pct) from a periodic task (every ~160 ms)
 * to animate the progress bar.  Call np_boot_finish() to skip
 * straight to setup (or wire to the "Pular" button event).
 *
 * Wiring example (FreeRTOS):
 *
 *   static void boot_task(void *arg)
 *   {
 *       for (uint8_t pct = 0; pct <= 100; ) {
 *           pct += (uint8_t)(esp_random() % 12 + 4);
 *           if (pct > 100) pct = 100;
 *           np_boot_advance(pct);
 *           // update bar widget here — see g_boot_bar below
 *           lv_bar_set_value(g_boot_bar, pct, LV_ANIM_ON);
 *           vTaskDelay(pdMS_TO_TICKS(160));
 *       }
 *       vTaskDelay(pdMS_TO_TICKS(500));
 *       np_boot_finish();
 *       vTaskDelete(NULL);
 *   }
 *   // after np_init():
 *   xTaskCreate(boot_task, "boot", 2048, NULL, 5, NULL);
 * ================================================================ */
#include "../novapanel.h"

lv_obj_t *g_boot_bar  = NULL;   /* expose so task can update */
lv_obj_t *g_boot_msg  = NULL;

static const char *boot_msg_for(uint8_t pct)
{
    if (pct < 30) return "Inicializando hardware...";
    if (pct < 60) return "Carregando sistema...";
    if (pct < 90) return "Verificando sensores...";
    return "Pronto!";
}

static void skip_cb(lv_event_t *e) { np_boot_finish(); }

void np_screen_boot(lv_obj_t *parent)
{
    lv_obj_t *scr = lv_obj_create(parent);
    /* full-panel overlay */
    lv_obj_set_size(scr, NP_W, NP_H);
    lv_obj_set_pos(scr, 0, 0);
    lv_obj_set_style_bg_color(scr,    lv_color_hex(0x060810), 0);
    lv_obj_set_style_bg_opa(scr,      LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(scr, 0, 0);
    lv_obj_set_style_radius(scr,       12, 0);
    lv_obj_set_style_pad_all(scr,      0, 0);
    lv_obj_set_scrollbar_mode(scr,     LV_SCROLLBAR_MODE_OFF);
    np_screens[NP_SCR_BOOT] = scr;

    /* ── centre column ── */
    lv_obj_t *col = lv_obj_create(scr);
    lv_obj_set_size(col, 300, LV_SIZE_CONTENT);
    lv_obj_align(col, LV_ALIGN_CENTER, 0, -10);
    np_obj_clear_style(col);
    lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(col,
        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(col, 0, 0);

    /* logo badge */
    lv_obj_t *badge = lv_obj_create(col);
    lv_obj_set_size(badge, 82, 82);
    lv_obj_set_style_bg_color(badge,     NP_C_ACCENT, 0);
    lv_obj_set_style_radius(badge,       20, 0);
    lv_obj_set_style_border_width(badge, 0, 0);
    lv_obj_set_style_pad_all(badge,      0, 0);
    lv_obj_set_style_margin_bottom(badge, 22, 0);
    lv_obj_set_scrollbar_mode(badge, LV_SCROLLBAR_MODE_OFF);
    lv_obj_t *badge_l = lv_label_create(badge);
    lv_label_set_text(badge_l, "NP");
    lv_obj_set_style_text_font(badge_l,  NP_F_3XL,    0);
    lv_obj_set_style_text_color(badge_l, NP_C_DARK_FG, 0);
    lv_obj_set_style_text_letter_space(badge_l, 2, 0);
    lv_obj_align(badge_l, LV_ALIGN_CENTER, 0, 0);

    /* product name */
    lv_obj_t *name = np_label(col, "NovaPanel", NP_F_3XL, NP_C_TEXT);
    lv_obj_set_style_margin_bottom(name, 6, 0);

    /* version */
    lv_obj_t *ver = np_label(col, "SISTEMA EMBARCADO \xc2\xb7 V1.3",
                             NP_F_XS, NP_C_TEXT_MUTED);
    lv_obj_set_style_text_letter_space(ver, 2, 0);
    lv_obj_set_style_margin_bottom(ver, 54, 0);

    /* progress bar track */
    lv_obj_t *track = lv_obj_create(col);
    lv_obj_set_size(track, 240, 3);
    lv_obj_set_style_bg_color(track,     lv_color_hex(0x1A1D2C), 0);
    lv_obj_set_style_bg_opa(track,       LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(track, 0, 0);
    lv_obj_set_style_radius(track,       2, 0);
    lv_obj_set_style_pad_all(track,      0, 0);
    lv_obj_set_style_margin_bottom(track, 16, 0);
    lv_obj_set_scrollbar_mode(track, LV_SCROLLBAR_MODE_OFF);

    /* actual bar */
    g_boot_bar = lv_bar_create(track);
    lv_obj_set_size(g_boot_bar, 240, 3);
    lv_obj_align(g_boot_bar, LV_ALIGN_LEFT_MID, 0, 0);
    lv_bar_set_range(g_boot_bar, 0, 100);
    lv_bar_set_value(g_boot_bar, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(g_boot_bar, lv_color_hex(0x1A1D2C), LV_PART_MAIN);
    lv_obj_set_style_bg_color(g_boot_bar, NP_C_ACCENT,             LV_PART_INDICATOR);
    lv_obj_set_style_radius(g_boot_bar,   2, LV_PART_MAIN);
    lv_obj_set_style_radius(g_boot_bar,   2, LV_PART_INDICATOR);

    /* status message */
    g_boot_msg = np_label(col, "Inicializando hardware...",
                          NP_F_XS, NP_C_TEXT_MUTED);
    lv_obj_set_style_text_letter_space(g_boot_msg, 1, 0);

    /* ── Skip button (bottom-right) ── */
    lv_obj_t *skip = lv_button_create(scr);
    lv_obj_set_size(skip, LV_SIZE_CONTENT, 38);
    lv_obj_align(skip, LV_ALIGN_BOTTOM_RIGHT, -24, -24);
    lv_obj_set_style_bg_color(skip,     lv_color_hex(0x0D0F18), 0);
    lv_obj_set_style_border_color(skip, NP_C_BORDER, 0);
    lv_obj_set_style_border_width(skip, 1, 0);
    lv_obj_set_style_radius(skip,       NP_R_BTN, 0);
    lv_obj_set_style_shadow_width(skip, 0, 0);
    lv_obj_set_style_pad_hor(skip, 16, 0);
    lv_obj_t *skip_l = np_label(skip, "Pular \xe2\x86\x92", NP_F_SM, NP_C_TEXT_MUTED);
    lv_obj_align(skip_l, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(skip, skip_cb, LV_EVENT_CLICKED, NULL);
}

/* Call from boot task after lv_bar_set_value() */
void np_boot_msg_update(void)
{
    if (g_boot_msg)
        lv_label_set_text(g_boot_msg, boot_msg_for(np_state.boot_pct));
}
