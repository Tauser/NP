/* ================================================================
 * SCREEN: Setup Wizard — LVGL v9.5
 *
 * 4 steps: Nome → Wi-Fi → Fuso/Formato → Confirmação
 * Call np_setup_next() / np_setup_back() to move between steps.
 * The wizard rebuilds the step content in-place when the user
 * navigates (hide/show child panels).
 * ================================================================ */
#include "../novapanel.h"
#include <string.h>
#include <stdio.h>

/* ── Step panels (pointers kept for show/hide) ────────────────── */
static lv_obj_t *g_step_panel[NP_SETUP_STEPS] = { NULL };
static lv_obj_t *g_prog_bar    = NULL;
static lv_obj_t *g_step_num    = NULL;
static lv_obj_t *g_step_title  = NULL;
static lv_obj_t *g_step_label  = NULL;
static lv_obj_t *g_back_btn    = NULL;
static lv_obj_t *g_next_btn    = NULL;
static lv_obj_t *g_next_lbl    = NULL;

/* Wi-Fi list items */
static const struct { const char *ssid; const char *sec; const char *sig; }
wifi_list[] = {
    { "NovaNet 5G",   "WPA2",   "-52 dBm" },
    { "NovaNet 2.4G", "WPA2",   "-61 dBm" },
    { "Vizinho_5G",   "WPA3",   "-74 dBm" },
    { "NET_8452",     "WPA2",   "-82 dBm" },
    { "CasaVizinho",  "Aberta", "-88 dBm" },
};
#define WIFI_N  (sizeof(wifi_list)/sizeof(wifi_list[0]))
static lv_obj_t *g_wifi_rows[WIFI_N]  = { NULL };
static lv_obj_t *g_wifi_check[WIFI_N] = { NULL };

/* Timezone list */
static const struct { const char *name; const char *offset; }
tz_list[] = {
    { "America/Sao_Paulo",   "UTC-3 \xc2\xb7 Brasil"          },
    { "America/New_York",    "UTC-5 \xc2\xb7 EUA Leste"       },
    { "America/Chicago",     "UTC-6 \xc2\xb7 EUA Central"     },
    { "America/Los_Angeles", "UTC-8 \xc2\xb7 EUA Pac\xc3\xad""fico" },
    { "Europe/London",       "UTC+0 \xc2\xb7 Reino Unido"     },
    { "Europe/Paris",        "UTC+1 \xc2\xb7 Europa Central"  },
    { "Asia/Tokyo",          "UTC+9 \xc2\xb7 Jap\xc3\xa3o"   },
    { "UTC",                 "UTC+0 \xc2\xb7 Universal"       },
};
#define TZ_N  (sizeof(tz_list)/sizeof(tz_list[0]))
static lv_obj_t *g_tz_rows[TZ_N]  = { NULL };
static lv_obj_t *g_tz_check[TZ_N] = { NULL };

/* Confirm rows */
static lv_obj_t *g_confirm_name = NULL;
static lv_obj_t *g_confirm_wifi = NULL;
static lv_obj_t *g_confirm_tz   = NULL;
static lv_obj_t *g_confirm_fmt  = NULL;

/* ── Helpers ──────────────────────────────────────────────────── */
static void setup_refresh_ui(void);

static void wifi_sel_cb(lv_event_t *e)
{
    int idx = (int)(uintptr_t)lv_event_get_user_data(e);
    lv_strlcpy(np_state.wifi_ssid, wifi_list[idx].ssid,
               sizeof(np_state.wifi_ssid));
    /* update check marks */
    for (int i = 0; i < (int)WIFI_N; i++) {
        if (g_wifi_check[i])
            (i == idx)
                ? lv_obj_clear_flag(g_wifi_check[i], LV_OBJ_FLAG_HIDDEN)
                : lv_obj_add_flag(g_wifi_check[i],   LV_OBJ_FLAG_HIDDEN);
        if (g_wifi_rows[i]) {
            lv_obj_set_style_bg_color(g_wifi_rows[i],
                i == idx ? NP_C_ACCENT_BG : NP_C_CARD, 0);
            lv_obj_set_style_border_color(g_wifi_rows[i],
                i == idx ? NP_C_ACCENT_BD : NP_C_BORDER, 0);
        }
    }
}

static void tz_sel_cb(lv_event_t *e)
{
    int idx = (int)(uintptr_t)lv_event_get_user_data(e);
    lv_strlcpy(np_state.setup_tz, tz_list[idx].name,
               sizeof(np_state.setup_tz));
    for (int i = 0; i < (int)TZ_N; i++) {
        if (g_tz_check[i])
            (i == idx)
                ? lv_obj_clear_flag(g_tz_check[i], LV_OBJ_FLAG_HIDDEN)
                : lv_obj_add_flag(g_tz_check[i],   LV_OBJ_FLAG_HIDDEN);
        if (g_tz_rows[i]) {
            lv_obj_set_style_bg_color(g_tz_rows[i],
                i == idx ? NP_C_ACCENT_BG : NP_C_CARD, 0);
            lv_obj_set_style_border_color(g_tz_rows[i],
                i == idx ? NP_C_ACCENT_BD : NP_C_BORDER, 0);
        }
    }
}

static void fmt24_cb(lv_event_t *e) { np_state.time_fmt_24h = true;  }
static void fmt12_cb(lv_event_t *e) { np_state.time_fmt_24h = false; }

static void next_cb(lv_event_t *e) { np_setup_next();  setup_refresh_ui(); }
static void back_cb(lv_event_t *e) { np_setup_back();  setup_refresh_ui(); }

static void name_ta_cb(lv_event_t *e)
{
    lv_obj_t *ta = lv_event_get_target(e);
    const char *t = lv_textarea_get_text(ta);
    lv_strlcpy(np_state.user_name, t, sizeof(np_state.user_name));
}

/* ── Refresh top bar + footer on step change ──────────────────── */
static const char *step_titles[NP_SETUP_STEPS] = {
    "Perfil", "Wi-Fi", "Fuso Hor\xc3\xa1rio", "Tudo certo!"
};
static const char *step_labels[NP_SETUP_STEPS] = {
    "Passo 1 de 3", "Passo 2 de 3", "Passo 3 de 3", ""
};
static const uint8_t step_pct[NP_SETUP_STEPS] = { 12, 40, 70, 100 };

static void setup_refresh_ui(void)
{
    int s = (int)np_state.setup_step;

    /* progress bar */
    if (g_prog_bar)
        lv_bar_set_value(g_prog_bar, step_pct[s], LV_ANIM_ON);

    /* step number */
    char num[4];
    if (s < 3) lv_snprintf(num, sizeof(num), "%d", s + 1);
    else       lv_strlcpy(num, "\xe2\x9c\x93", sizeof(num));
    if (g_step_num)   lv_label_set_text(g_step_num,   num);
    if (g_step_title) lv_label_set_text(g_step_title, step_titles[s]);
    if (g_step_label) lv_label_set_text(g_step_label, step_labels[s]);

    /* show/hide step panels */
    for (int i = 0; i < NP_SETUP_STEPS; i++) {
        if (!g_step_panel[i]) continue;
        (i == s) ? lv_obj_clear_flag(g_step_panel[i], LV_OBJ_FLAG_HIDDEN)
                 : lv_obj_add_flag(g_step_panel[i],   LV_OBJ_FLAG_HIDDEN);
    }

    /* back button visibility */
    if (g_back_btn)
        (s > 0) ? lv_obj_clear_flag(g_back_btn, LV_OBJ_FLAG_HIDDEN)
                : lv_obj_add_flag(g_back_btn,   LV_OBJ_FLAG_HIDDEN);

    /* next label */
    if (g_next_lbl)
        lv_label_set_text(g_next_lbl,
            s < (NP_SETUP_STEPS - 1)
                ? "Continuar \xe2\x86\x92"
                : "Iniciar NovaPanel \xe2\x86\x92");
    if (g_next_btn)
        lv_obj_set_style_bg_color(g_next_btn,
            s == (NP_SETUP_STEPS - 1) ? NP_C_GREEN : NP_C_ACCENT, 0);

    /* update confirm panel when visible */
    if (s == NP_SETUP_DONE) {
        if (g_confirm_name) lv_label_set_text(g_confirm_name, np_state.user_name[0] ? np_state.user_name : "—");
        if (g_confirm_wifi) lv_label_set_text(g_confirm_wifi, np_state.wifi_ssid);
        if (g_confirm_tz)   lv_label_set_text(g_confirm_tz,   np_state.setup_tz);
        if (g_confirm_fmt)  lv_label_set_text(g_confirm_fmt,  np_state.time_fmt_24h ? "24 horas" : "12 horas");
    }
}

/* ── confirm row helper ───────────────────────────────────────── */
static lv_obj_t *confirm_row(lv_obj_t *parent, const char *key,
                              const char *val, lv_color_t vcol)
{
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_set_size(row, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(row,     NP_C_CARD, 0);
    lv_obj_set_style_bg_opa(row,       LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(row, NP_C_BORDER, 0);
    lv_obj_set_style_border_width(row, 1, 0);
    lv_obj_set_style_radius(row,       9, 0);
    lv_obj_set_style_pad_all(row,      12, 0);
    lv_obj_set_scrollbar_mode(row, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row,
        LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_margin_bottom(row, 8, 0);

    np_label(row, key, NP_F_SM, NP_C_TEXT_DIM);
    return np_label(row, val, NP_F_SM, vcol);
}

/* ================================================================
 * Builder
 * ================================================================ */
void np_screen_setup(lv_obj_t *parent)
{
    lv_obj_t *scr = lv_obj_create(parent);
    lv_obj_set_size(scr, NP_W, NP_H);
    lv_obj_set_pos(scr, 0, 0);
    lv_obj_set_style_bg_color(scr,    NP_C_BG, 0);
    lv_obj_set_style_bg_opa(scr,      LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(scr, 0, 0);
    lv_obj_set_style_radius(scr,       12, 0);
    lv_obj_set_style_pad_all(scr,      0, 0);
    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(scr,
        LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_scrollbar_mode(scr, LV_SCROLLBAR_MODE_OFF);
    np_screens[NP_SCR_SETUP] = scr;

    /* ── progress bar ── */
    lv_obj_t *prog_track = lv_obj_create(scr);
    lv_obj_set_size(prog_track, lv_pct(100), 3);
    lv_obj_set_style_bg_color(prog_track,     lv_color_hex(0x1A1D2C), 0);
    lv_obj_set_style_bg_opa(prog_track,       LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(prog_track, 0, 0);
    lv_obj_set_style_radius(prog_track,       0, 0);
    lv_obj_set_style_pad_all(prog_track,      0, 0);
    lv_obj_set_scrollbar_mode(prog_track, LV_SCROLLBAR_MODE_OFF);

    g_prog_bar = lv_bar_create(prog_track);
    lv_obj_set_size(g_prog_bar, lv_pct(100), 3);
    lv_bar_set_range(g_prog_bar, 0, 100);
    lv_bar_set_value(g_prog_bar, step_pct[0], LV_ANIM_OFF);
    lv_obj_set_style_bg_color(g_prog_bar, lv_color_hex(0x1A1D2C), LV_PART_MAIN);
    lv_obj_set_style_bg_color(g_prog_bar, NP_C_ACCENT,             LV_PART_INDICATOR);
    lv_obj_set_style_radius(g_prog_bar,   0, LV_PART_MAIN);
    lv_obj_set_style_radius(g_prog_bar,   0, LV_PART_INDICATOR);

    /* ── step header ── */
    lv_obj_t *hdr = lv_obj_create(scr);
    lv_obj_set_size(hdr, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(hdr,     LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(hdr, 0, 0);
    lv_obj_set_style_pad_all(hdr,    16, 0);
    lv_obj_set_style_pad_bottom(hdr, 0, 0);
    lv_obj_set_flex_flow(hdr, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(hdr,
        LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(hdr, 10, 0);
    lv_obj_set_scrollbar_mode(hdr, LV_SCROLLBAR_MODE_OFF);

    lv_obj_t *num_badge = lv_obj_create(hdr);
    lv_obj_set_size(num_badge, 30, 30);
    lv_obj_set_style_bg_color(num_badge,     NP_C_ACCENT_BG, 0);
    lv_obj_set_style_border_color(num_badge, NP_C_ACCENT_BD, 0);
    lv_obj_set_style_border_width(num_badge, 1, 0);
    lv_obj_set_style_radius(num_badge,       15, 0);
    lv_obj_set_style_pad_all(num_badge,      0, 0);
    lv_obj_set_scrollbar_mode(num_badge, LV_SCROLLBAR_MODE_OFF);
    g_step_num = lv_label_create(num_badge);
    lv_label_set_text(g_step_num, "1");
    lv_obj_set_style_text_font(g_step_num,  NP_F_SM, 0);
    lv_obj_set_style_text_color(g_step_num, NP_C_ACCENT, 0);
    lv_obj_align(g_step_num, LV_ALIGN_CENTER, 0, 0);

    g_step_title = np_label(hdr, step_titles[0], NP_F_LG, NP_C_ACCENT);

    lv_obj_t *hdr_sp = lv_obj_create(hdr);
    np_obj_clear_style(hdr_sp);
    lv_obj_set_style_flex_grow(hdr_sp, 1, 0);

    g_step_label = np_label(hdr, step_labels[0], NP_F_XS, NP_C_TEXT_MUTED);

    /* ── step content area (flex:1) ── */
    lv_obj_t *body = lv_obj_create(scr);
    lv_obj_set_size(body, lv_pct(100), 0);
    lv_obj_set_style_flex_grow(body, 1, 0);
    np_obj_clear_style(body);
    lv_obj_set_scrollbar_mode(body, LV_SCROLLBAR_MODE_OFF);

    /* ─────────────── STEP 0: Nome ─────────────── */
    g_step_panel[NP_SETUP_NAME] = lv_obj_create(body);
    lv_obj_set_size(g_step_panel[0], lv_pct(100), lv_pct(100));
    np_obj_clear_style(g_step_panel[0]);
    lv_obj_set_flex_flow(g_step_panel[0], LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(g_step_panel[0],
        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(g_step_panel[0], 0, 0);
    lv_obj_set_style_pad_hor(g_step_panel[0], 80, 0);

    lv_obj_t *avatar = lv_obj_create(g_step_panel[0]);
    lv_obj_set_size(avatar, 76, 76);
    lv_obj_set_style_bg_color(avatar,     NP_C_ACCENT_BG, 0);
    lv_obj_set_style_bg_opa(avatar,       LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(avatar, NP_C_ACCENT, 0);
    lv_obj_set_style_border_width(avatar, 2, 0);
    lv_obj_set_style_radius(avatar,       38, 0);
    lv_obj_set_style_pad_all(avatar,      0, 0);
    lv_obj_set_style_margin_bottom(avatar, 22, 0);
    lv_obj_set_scrollbar_mode(avatar, LV_SCROLLBAR_MODE_OFF);
    lv_obj_t *av_l = np_label(avatar, "?", NP_F_3XL, NP_C_ACCENT);
    lv_obj_align(av_l, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t *q = np_label(g_step_panel[0],
        "Como quer ser chamado?", NP_F_2XL, NP_C_TEXT);
    lv_obj_set_style_text_align(q, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_margin_bottom(q, 10, 0);

    lv_obj_t *sub = np_label(g_step_panel[0],
        "Seu nome apar\xc3\xa9""cer\xc3\xa1 nas sauda\xc3\xa7\xc3\xb5""es\ne no perfil do dispositivo.",
        NP_F_SM, NP_C_TEXT_DIM);
    lv_obj_set_style_text_align(sub, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_margin_bottom(sub, 30, 0);

    lv_obj_t *ta = lv_textarea_create(g_step_panel[0]);
    lv_obj_set_width(ta, 340);
    lv_textarea_set_one_line(ta, true);
    lv_textarea_set_placeholder_text(ta, "Seu nome");
    lv_obj_set_style_bg_color(ta,     NP_C_CARD, 0);
    lv_obj_set_style_border_color(ta, NP_C_ACCENT_BD, 0);
    lv_obj_set_style_border_width(ta, 1, 0);
    lv_obj_set_style_radius(ta,       12, 0);
    lv_obj_set_style_text_font(ta,    NP_F_XL, 0);
    lv_obj_set_style_text_color(ta,   NP_C_TEXT, 0);
    lv_obj_set_style_text_align(ta,   LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_add_event_cb(ta, name_ta_cb, LV_EVENT_VALUE_CHANGED, NULL);

    /* ─────────────── STEP 1: Wi-Fi ─────────────── */
    g_step_panel[NP_SETUP_WIFI] = lv_obj_create(body);
    lv_obj_set_size(g_step_panel[1], lv_pct(100), lv_pct(100));
    np_obj_clear_style(g_step_panel[1]);
    lv_obj_set_flex_flow(g_step_panel[1], LV_FLEX_FLOW_ROW);
    lv_obj_add_flag(g_step_panel[1], LV_OBJ_FLAG_HIDDEN);

    /* network list */
    lv_obj_t *wlist = lv_obj_create(g_step_panel[1]);
    lv_obj_set_style_flex_grow(wlist, 1, 0);
    lv_obj_set_height(wlist, lv_pct(100));
    lv_obj_set_style_bg_opa(wlist,     LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(wlist, 0, 0);
    lv_obj_set_style_pad_all(wlist,    16, 0);
    lv_obj_set_flex_flow(wlist, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(wlist,
        LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_row(wlist, 7, 0);
    lv_obj_set_scrollbar_mode(wlist, LV_SCROLLBAR_MODE_ON);

    lv_obj_t *wl_hdr = np_label(wlist,
        "Selecione a rede Wi-Fi do seu ambiente:",
        NP_F_SM, NP_C_TEXT_DIM);
    lv_obj_set_style_margin_bottom(wl_hdr, 7, 0);

    for (int i = 0; i < (int)WIFI_N; i++) {
        bool sel = (strcmp(wifi_list[i].ssid, np_state.wifi_ssid) == 0);
        lv_obj_t *row = lv_obj_create(wlist);
        lv_obj_set_size(row, lv_pct(100), LV_SIZE_CONTENT);
        lv_obj_set_style_bg_color(row,     sel ? NP_C_ACCENT_BG : NP_C_CARD, 0);
        lv_obj_set_style_border_color(row, sel ? NP_C_ACCENT_BD : NP_C_BORDER, 0);
        lv_obj_set_style_border_width(row, 1, 0);
        lv_obj_set_style_radius(row,       10, 0);
        lv_obj_set_style_pad_all(row,      13, 0);
        lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(row,
            LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_column(row, 12, 0);
        lv_obj_set_scrollbar_mode(row, LV_SCROLLBAR_MODE_OFF);

        lv_obj_t *ic_box = lv_obj_create(row);
        lv_obj_set_size(ic_box, 38, 38);
        lv_obj_set_style_bg_color(ic_box,     NP_C_CARD2, 0);
        lv_obj_set_style_border_width(ic_box, 0, 0);
        lv_obj_set_style_radius(ic_box,       9, 0);
        lv_obj_set_style_pad_all(ic_box,      0, 0);
        lv_obj_set_scrollbar_mode(ic_box, LV_SCROLLBAR_MODE_OFF);
        lv_obj_t *ic = np_label(ic_box, LV_SYMBOL_WIFI,
                                NP_F_LG, sel ? NP_C_ACCENT : NP_C_TEXT_DIM);
        lv_obj_align(ic, LV_ALIGN_CENTER, 0, 0);

        lv_obj_t *info = np_col(row);
        lv_obj_set_style_flex_grow(info, 1, 0);
        lv_obj_set_size(info, 0, LV_SIZE_CONTENT);
        np_label(info, wifi_list[i].ssid, NP_F_MD,
                 sel ? NP_C_TEXT : lv_color_hex(0xC4C8D6));
        char sub2[64];
        lv_snprintf(sub2, sizeof(sub2), "%s \xc2\xb7 %s",
                    wifi_list[i].sec, wifi_list[i].sig);
        lv_obj_t *sl2 = np_label(info, sub2, NP_F_XS, NP_C_TEXT_MUTED);
        lv_obj_set_style_margin_top(sl2, 2, 0);

        lv_obj_t *chk = lv_obj_create(row);
        lv_obj_set_size(chk, 22, 22);
        lv_obj_set_style_bg_color(chk,     NP_C_ACCENT, 0);
        lv_obj_set_style_border_width(chk, 0, 0);
        lv_obj_set_style_radius(chk,       11, 0);
        lv_obj_set_style_pad_all(chk,      0, 0);
        lv_obj_set_scrollbar_mode(chk, LV_SCROLLBAR_MODE_OFF);
        lv_obj_t *chk_ic = np_label(chk, LV_SYMBOL_OK,
                                    NP_F_XS, NP_C_DARK_FG);
        lv_obj_align(chk_ic, LV_ALIGN_CENTER, 0, 0);
        if (!sel) lv_obj_add_flag(chk, LV_OBJ_FLAG_HIDDEN);

        lv_obj_add_event_cb(row, wifi_sel_cb, LV_EVENT_CLICKED,
                            (void *)(uintptr_t)i);
        g_wifi_rows[i]  = row;
        g_wifi_check[i] = chk;
    }

    /* password side panel */
    lv_obj_t *wpanel = lv_obj_create(g_step_panel[1]);
    lv_obj_set_size(wpanel, 258, lv_pct(100));
    lv_obj_set_style_bg_color(wpanel,     NP_C_CARD, 0);
    lv_obj_set_style_bg_opa(wpanel,       LV_OPA_COVER, 0);
    lv_obj_set_style_border_side(wpanel,  LV_BORDER_SIDE_LEFT, 0);
    lv_obj_set_style_border_color(wpanel, NP_C_BORDER, 0);
    lv_obj_set_style_border_width(wpanel, 1, 0);
    lv_obj_set_style_radius(wpanel,       0, 0);
    lv_obj_set_style_pad_all(wpanel,      24, 0);
    lv_obj_set_flex_flow(wpanel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(wpanel,
        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_row(wpanel, 0, 0);
    lv_obj_set_scrollbar_mode(wpanel, LV_SCROLLBAR_MODE_OFF);

    np_label(wpanel, "REDE SELECIONADA",
             NP_F_XS, NP_C_TEXT_MUTED);
    lv_obj_t *ssid_lbl = np_label(wpanel, np_state.wifi_ssid, NP_F_LG, NP_C_TEXT);
    lv_obj_set_style_margin_top(ssid_lbl, 8, 0);
    lv_obj_set_style_margin_bottom(ssid_lbl, 20, 0);

    np_label(wpanel, "Senha", NP_F_SM, NP_C_TEXT_DIM);

    lv_obj_t *pass_ta = lv_textarea_create(wpanel);
    lv_obj_set_width(pass_ta, lv_pct(100));
    lv_textarea_set_one_line(pass_ta, true);
    lv_textarea_set_password_mode(pass_ta, true);
    lv_textarea_set_placeholder_text(pass_ta, "Senha da rede");
    lv_obj_set_style_bg_color(pass_ta,     NP_C_CARD2, 0);
    lv_obj_set_style_border_color(pass_ta, NP_C_BORDER, 0);
    lv_obj_set_style_border_width(pass_ta, 1, 0);
    lv_obj_set_style_radius(pass_ta,       9, 0);
    lv_obj_set_style_text_font(pass_ta,    NP_F_LG, 0);
    lv_obj_set_style_text_color(pass_ta,   NP_C_TEXT, 0);
    lv_obj_set_style_margin_top(pass_ta,   8, 0);
    lv_obj_set_style_margin_bottom(pass_ta, 14, 0);

    lv_obj_t *hint = np_label(wpanel,
        "Deixe em branco para\nredes abertas.",
        NP_F_XS, NP_C_TEXT_MUTED);
    lv_obj_set_style_text_align(hint, LV_TEXT_ALIGN_LEFT, 0);

    /* ─────────────── STEP 2: Fuso + Formato ─────────────── */
    g_step_panel[NP_SETUP_TZ] = lv_obj_create(body);
    lv_obj_set_size(g_step_panel[2], lv_pct(100), lv_pct(100));
    np_obj_clear_style(g_step_panel[2]);
    lv_obj_set_flex_flow(g_step_panel[2], LV_FLEX_FLOW_ROW);
    lv_obj_add_flag(g_step_panel[2], LV_OBJ_FLAG_HIDDEN);

    lv_obj_t *tzlist = lv_obj_create(g_step_panel[2]);
    lv_obj_set_style_flex_grow(tzlist, 1, 0);
    lv_obj_set_height(tzlist, lv_pct(100));
    lv_obj_set_style_bg_opa(tzlist,     LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(tzlist, 0, 0);
    lv_obj_set_style_pad_all(tzlist,    16, 0);
    lv_obj_set_flex_flow(tzlist, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(tzlist, 6, 0);
    lv_obj_set_scrollbar_mode(tzlist, LV_SCROLLBAR_MODE_ON);

    lv_obj_t *tz_hdr = np_label(tzlist, "Escolha seu fuso hor\xc3\xa1rio:",
                                NP_F_SM, NP_C_TEXT_DIM);
    lv_obj_set_style_margin_bottom(tz_hdr, 8, 0);

    for (int i = 0; i < (int)TZ_N; i++) {
        bool a = (strcmp(tz_list[i].name, np_state.setup_tz) == 0);
        lv_obj_t *row = lv_obj_create(tzlist);
        lv_obj_set_size(row, lv_pct(100), LV_SIZE_CONTENT);
        lv_obj_set_style_bg_color(row,     a ? NP_C_ACCENT_BG : NP_C_CARD, 0);
        lv_obj_set_style_border_color(row, a ? NP_C_ACCENT_BD : NP_C_BORDER, 0);
        lv_obj_set_style_border_width(row, 1, 0);
        lv_obj_set_style_radius(row,       9, 0);
        lv_obj_set_style_pad_all(row,      12, 0);
        lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(row,
            LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_scrollbar_mode(row, LV_SCROLLBAR_MODE_OFF);

        lv_obj_t *ti = np_col(row);
        lv_obj_set_style_flex_grow(ti, 1, 0);
        lv_obj_set_size(ti, 0, LV_SIZE_CONTENT);
        np_label(ti, tz_list[i].name,   NP_F_MD, a ? NP_C_TEXT : lv_color_hex(0xC4C8D6));
        lv_obj_t *toff = np_label(ti, tz_list[i].offset, NP_F_XS, NP_C_TEXT_MUTED);
        lv_obj_set_style_margin_top(toff, 2, 0);

        lv_obj_t *chk = lv_obj_create(row);
        lv_obj_set_size(chk, 20, 20);
        lv_obj_set_style_bg_color(chk,     NP_C_ACCENT, 0);
        lv_obj_set_style_border_width(chk, 0, 0);
        lv_obj_set_style_radius(chk,       10, 0);
        lv_obj_set_style_pad_all(chk,      0, 0);
        lv_obj_set_scrollbar_mode(chk, LV_SCROLLBAR_MODE_OFF);
        lv_obj_t *chk_i = np_label(chk, LV_SYMBOL_OK, NP_F_XS, NP_C_DARK_FG);
        lv_obj_align(chk_i, LV_ALIGN_CENTER, 0, 0);
        if (!a) lv_obj_add_flag(chk, LV_OBJ_FLAG_HIDDEN);

        lv_obj_add_event_cb(row, tz_sel_cb, LV_EVENT_CLICKED,
                            (void *)(uintptr_t)i);
        g_tz_rows[i]  = row;
        g_tz_check[i] = chk;
    }

    /* format side panel */
    lv_obj_t *fpanel = lv_obj_create(g_step_panel[2]);
    lv_obj_set_size(fpanel, 258, lv_pct(100));
    lv_obj_set_style_bg_color(fpanel,     NP_C_CARD, 0);
    lv_obj_set_style_bg_opa(fpanel,       LV_OPA_COVER, 0);
    lv_obj_set_style_border_side(fpanel,  LV_BORDER_SIDE_LEFT, 0);
    lv_obj_set_style_border_color(fpanel, NP_C_BORDER, 0);
    lv_obj_set_style_border_width(fpanel, 1, 0);
    lv_obj_set_style_radius(fpanel,       0, 0);
    lv_obj_set_style_pad_all(fpanel,      24, 0);
    lv_obj_set_flex_flow(fpanel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(fpanel, 0, 0);
    lv_obj_set_scrollbar_mode(fpanel, LV_SCROLLBAR_MODE_OFF);

    np_label(fpanel, "Formato de hora", NP_F_MD, NP_C_TEXT);
    lv_obj_t *fmt_col = lv_obj_create(fpanel);
    np_obj_clear_style(fmt_col);
    lv_obj_set_size(fmt_col, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_style_margin_top(fmt_col, 12, 0);
    lv_obj_set_style_margin_bottom(fmt_col, 20, 0);
    lv_obj_set_flex_flow(fmt_col, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(fmt_col, 8, 0);

    for (int i = 0; i < 2; i++) {
        bool active_fmt = (i == 0) == np_state.time_fmt_24h;
        lv_obj_t *fb = lv_obj_create(fmt_col);
        lv_obj_set_size(fb, lv_pct(100), LV_SIZE_CONTENT);
        lv_obj_set_style_bg_color(fb,     active_fmt ? NP_C_ACCENT_BG : NP_C_CARD2, 0);
        lv_obj_set_style_border_color(fb, active_fmt ? NP_C_ACCENT_BD : NP_C_BORDER, 0);
        lv_obj_set_style_border_width(fb, 1, 0);
        lv_obj_set_style_radius(fb,       9, 0);
        lv_obj_set_style_pad_all(fb,      12, 0);
        lv_obj_set_flex_flow(fb, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_scrollbar_mode(fb, LV_SCROLLBAR_MODE_OFF);
        lv_obj_t *fl = np_label(fb, i == 0 ? "24 horas" : "12 horas",
                                NP_F_LG, active_fmt ? NP_C_ACCENT : NP_C_TEXT);
        lv_obj_set_style_margin_bottom(fl, 3, 0);
        np_label(fb, i == 0 ? "Ex: 14:30" : "Ex: 2:30 PM",
                 NP_F_XS, NP_C_TEXT_MUTED);
        lv_obj_add_event_cb(fb,
            i == 0 ? fmt24_cb : fmt12_cb, LV_EVENT_CLICKED, NULL);
    }

    np_hsep(fpanel);
    lv_obj_t *tz_sel_lbl = np_label(fpanel,
        "FUSO SELECIONADO", NP_F_XS, NP_C_TEXT_MUTED);
    lv_obj_set_style_margin_top(tz_sel_lbl, 16, 0);
    lv_obj_set_style_margin_bottom(tz_sel_lbl, 8, 0);
    np_label(fpanel, np_state.setup_tz, NP_F_SM, NP_C_ACCENT);

    /* ─────────────── STEP 3: Confirmação ─────────────── */
    g_step_panel[NP_SETUP_DONE] = lv_obj_create(body);
    lv_obj_set_size(g_step_panel[3], lv_pct(100), lv_pct(100));
    np_obj_clear_style(g_step_panel[3]);
    lv_obj_set_flex_flow(g_step_panel[3], LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(g_step_panel[3],
        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(g_step_panel[3], 0, 0);
    lv_obj_set_style_pad_hor(g_step_panel[3], 60, 0);
    lv_obj_add_flag(g_step_panel[3], LV_OBJ_FLAG_HIDDEN);

    lv_obj_t *ok_badge = lv_obj_create(g_step_panel[3]);
    lv_obj_set_size(ok_badge, 72, 72);
    lv_obj_set_style_bg_color(ok_badge,     NP_C_GREEN_BG, 0);
    lv_obj_set_style_border_color(ok_badge, NP_C_GREEN, 0);
    lv_obj_set_style_border_width(ok_badge, 2, 0);
    lv_obj_set_style_radius(ok_badge,       36, 0);
    lv_obj_set_style_pad_all(ok_badge,      0, 0);
    lv_obj_set_style_margin_bottom(ok_badge, 14, 0);
    lv_obj_set_scrollbar_mode(ok_badge, LV_SCROLLBAR_MODE_OFF);
    lv_obj_t *ok_ic = np_label(ok_badge, LV_SYMBOL_OK, NP_F_3XL, NP_C_GREEN);
    lv_obj_align(ok_ic, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t *done_title = np_label(g_step_panel[3],
        "Tudo pronto!", NP_F_3XL, NP_C_TEXT);
    lv_obj_set_style_margin_bottom(done_title, 8, 0);

    lv_obj_t *done_sub = np_label(g_step_panel[3],
        "Seu NovaPanel est\xc3\xa1 configurado. Voc\xc3\xaa\npode alterar dados depois em Configura\xc3\xa7\xc3\xb5""es.",
        NP_F_SM, NP_C_TEXT_DIM);
    lv_obj_set_style_text_align(done_sub, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_margin_bottom(done_sub, 20, 0);

    lv_obj_t *cr_wrap = lv_obj_create(g_step_panel[3]);
    lv_obj_set_size(cr_wrap, 360, LV_SIZE_CONTENT);
    np_obj_clear_style(cr_wrap);
    lv_obj_set_flex_flow(cr_wrap, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(cr_wrap, 0, 0);

    g_confirm_name = confirm_row(cr_wrap, "Nome", "—",        NP_C_TEXT);
    g_confirm_wifi = confirm_row(cr_wrap, "Wi-Fi", "—",       NP_C_GREEN);
    g_confirm_tz   = confirm_row(cr_wrap, "Fuso", "—",        NP_C_TEXT);
    g_confirm_fmt  = confirm_row(cr_wrap, "Hora", "24 horas", NP_C_TEXT);

    /* ── Footer nav ── */
    lv_obj_t *footer = lv_obj_create(scr);
    lv_obj_set_size(footer, lv_pct(100), 62);
    lv_obj_set_style_bg_color(footer,     NP_C_BG, 0);
    lv_obj_set_style_bg_opa(footer,       LV_OPA_COVER, 0);
    lv_obj_set_style_border_side(footer,  LV_BORDER_SIDE_TOP, 0);
    lv_obj_set_style_border_color(footer, NP_C_BORDER, 0);
    lv_obj_set_style_border_width(footer, 1, 0);
    lv_obj_set_style_radius(footer,       0, 0);
    lv_obj_set_style_pad_all(footer,      14, 0);
    lv_obj_set_flex_flow(footer, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(footer,
        LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(footer, 12, 0);
    lv_obj_set_scrollbar_mode(footer, LV_SCROLLBAR_MODE_OFF);

    g_back_btn = lv_button_create(footer);
    lv_obj_set_size(g_back_btn, LV_SIZE_CONTENT, 46);
    lv_obj_set_style_bg_color(g_back_btn,     NP_C_CARD2, 0);
    lv_obj_set_style_border_color(g_back_btn, NP_C_BORDER, 0);
    lv_obj_set_style_border_width(g_back_btn, 1, 0);
    lv_obj_set_style_radius(g_back_btn,       NP_R_BTN, 0);
    lv_obj_set_style_shadow_width(g_back_btn, 0, 0);
    lv_obj_set_style_pad_hor(g_back_btn,      22, 0);
    lv_obj_t *back_l = np_label(g_back_btn, "\xe2\x86\x90 Voltar",
                                NP_F_MD, NP_C_TEXT_DIM);
    lv_obj_align(back_l, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(g_back_btn, back_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_flag(g_back_btn, LV_OBJ_FLAG_HIDDEN);  /* hidden on step 0 */

    lv_obj_t *sp = lv_obj_create(footer);
    np_obj_clear_style(sp);
    lv_obj_set_style_flex_grow(sp, 1, 0);

    g_next_btn = lv_button_create(footer);
    lv_obj_set_size(g_next_btn, LV_SIZE_CONTENT, 46);
    lv_obj_set_style_bg_color(g_next_btn,     NP_C_ACCENT, 0);
    lv_obj_set_style_radius(g_next_btn,       NP_R_BTN, 0);
    lv_obj_set_style_shadow_width(g_next_btn, 0, 0);
    lv_obj_set_style_pad_hor(g_next_btn,      24, 0);
    g_next_lbl = np_label(g_next_btn, "Continuar \xe2\x86\x92",
                          NP_F_MD, NP_C_DARK_FG);
    lv_obj_align(g_next_lbl, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(g_next_btn, next_cb, LV_EVENT_CLICKED, NULL);
}
