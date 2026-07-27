/* ================================================================
 * SCREEN: Notificações — LVGL v9.5
 *   • lv_obj_set_style_flex_grow() instead of lv_obj_set_flex_grow()
 * ================================================================ */
#include "../novapanel.h"

void np_screen_notif(lv_obj_t *parent)
{
    lv_obj_t *scr = lv_obj_create(parent);
    lv_obj_set_size(scr, lv_pct(100), lv_pct(100));
    lv_obj_set_pos(scr, 0, 0);
    np_obj_clear_style(scr);
    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(scr,
        LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_row(scr, 9, 0);
    lv_obj_set_scrollbar_mode(scr, LV_SCROLLBAR_MODE_ON);
    np_screens[NP_SCR_NOTIF] = scr;

    static const struct { const char *title; const char *desc; const char *time; uint32_t c; }
    notifs[] = {
        { "Alarme: 07:00",
          "Alarme 'Acordar' disparado e confirmado.",
          "agora", 0xE8A83C },
        { "Reunião em 1h 12min",
          "NoiseBot team · Sala virtual · 15:30",
          "09:30", 0x4F7ECB },
        { "Firmware v1.4 disponível",
          "Atualização de segurança — 2,1 MB",
          "08:15", 0x4ABB78 },
        { "Sensor CO₂ elevado",
          "Nível de CO₂ acima de 800 ppm na sala.",
          "07:58", 0xD05252 },
        { "Wi-Fi reconectado",
          "NovaNet 5G · -52 dBm · 192.168.0.114",
          "06:42", 0x4ABB78 },
        { "Cena 'Bom dia' ativada",
          "Luzes da sala ligadas · volume 65%",
          "06:18", 0xB77ABB },
    };
    for (int i = 0; i < 6; i++) {
        lv_obj_t *card = np_card(scr);
        lv_obj_set_size(card, lv_pct(100), LV_SIZE_CONTENT);
        lv_obj_set_style_pad_hor(card, 16, 0);
        lv_obj_set_style_pad_ver(card, 14, 0);

        lv_obj_t *row = np_row(card);
        lv_obj_set_flex_align(row,
            LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
        lv_obj_set_style_pad_column(row, 12, 0);

        lv_obj_t *dot = lv_obj_create(row);
        lv_obj_set_size(dot, 8, 8);
        lv_obj_set_style_bg_color(dot,     lv_color_hex(notifs[i].c), 0);
        lv_obj_set_style_bg_opa(dot,       LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(dot, 0, 0);
        lv_obj_set_style_radius(dot,       4, 0);
        lv_obj_set_style_margin_top(dot,   4, 0);

        lv_obj_t *info = np_col(row);
        lv_obj_set_style_flex_grow(info, 1, 0);  /* v9 */
        lv_obj_set_size(info, 0, LV_SIZE_CONTENT);
        np_label(info, notifs[i].title, NP_F_MD, NP_C_TEXT);
        lv_obj_t *desc = np_label(info, notifs[i].desc, NP_F_SM,
                                  lv_color_hex(0x7A8298));
        lv_obj_set_style_margin_top(desc, 3, 0);

        np_label(row, notifs[i].time, NP_F_XS, NP_C_TEXT_MUTED);
    }
}
