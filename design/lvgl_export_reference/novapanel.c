/* ================================================================
 * NOVAPANEL — Core  (v2)
 * Changes vs v1:
 *   • Rail shows icon ONLY — no text label under each button
 *   • NP_TOPBAR_H 54 → 60 px
 *   • Boot + Setup phase management
 *   • Screen-indicator dot strip in normal flow (not absolute)
 * LVGL v9.5
 * ================================================================ */
#include "novapanel.h"
#include <stdio.h>
#include <time.h>

/* ── Global state ─────────────────────────────────────────────── */
np_state_t np_state = {
    .phase          = NP_PHASE_BOOT,
    .boot_pct       = 0,
    .setup_step     = NP_SETUP_NAME,
    .user_name      = "",
    .wifi_ssid      = "NovaNet 5G",
    .wifi_pass      = "",
    .setup_tz       = "America/Sao_Paulo",
    .time_fmt_24h   = true,
    .current        = NP_SCR_HOME,
    .night_mode     = false,
    .bt_enabled     = false,
    .brilho         = 78,
    .vol_system     = 65,
    .vol_music      = 80,
    .vol_alarm      = 90,
    .timer_sec      = 1500,
    .timer_total    = 1500,
    .timer_running  = false,
    .dev_luz_sala    = true,
    .dev_luz_cozinha = false,
    .dev_luz_quarto  = false,
    .dev_tomada_tv   = true,
    .dev_tomada_cafe = false,
    .dev_ventilador  = false,
    .alarm_on        = { true, true, false, true },
};

/* ── Root objects ─────────────────────────────────────────────── */
lv_obj_t *np_root                      = NULL;
lv_obj_t *np_rail                      = NULL;
lv_obj_t *np_topbar                    = NULL;
lv_obj_t *np_content                   = NULL;
lv_obj_t *np_dots_bar                  = NULL;
lv_obj_t *np_screens[NP_SCR_TOTAL]     = { NULL };

/* ── Private ──────────────────────────────────────────────────── */
static lv_obj_t  *rail_btn[NP_SCR_COUNT] = { NULL };
static lv_obj_t  *topbar_title           = NULL;
static lv_obj_t  *topbar_clock           = NULL;
static lv_obj_t  *dot_objs[NP_SCR_COUNT] = { NULL };

static const char *scr_titles[NP_SCR_COUNT] = {
    "Início", "Agenda", "Mercado", "Casa",
    "Alarmes", "Clima", "Pomodoro",
    "Configurações", "Notificações",
};

/* icon-only nav entries */
static const struct {
    const char  *sym;
    np_screen_t  scr;
} nav_items[] = {
    { LV_SYMBOL_HOME,    NP_SCR_HOME    },
    { LV_SYMBOL_LIST,    NP_SCR_AGENDA  },
    { LV_SYMBOL_CHARGE,  NP_SCR_MERCADO },
    { LV_SYMBOL_IMAGE,   NP_SCR_CASA    },
    { LV_SYMBOL_BELL,    NP_SCR_ALARMES },
    { LV_SYMBOL_TINT,    NP_SCR_CLIMA   },
    { LV_SYMBOL_REFRESH, NP_SCR_TIMER   },
};
#define NAV_N  (sizeof(nav_items)/sizeof(nav_items[0]))

/* ── Callbacks ────────────────────────────────────────────────── */
static void nav_cb(lv_event_t *e)
{
    np_navigate_to((np_screen_t)(uintptr_t)lv_event_get_user_data(e));
}
static void gear_cb(lv_event_t *e) { np_navigate_to(NP_SCR_CONFIG); }
static void bell_cb(lv_event_t *e) { np_navigate_to(NP_SCR_NOTIF);  }

/* ── Dot update ───────────────────────────────────────────────── */
static void dots_update(void)
{
    for (int i = 0; i < NP_SCR_COUNT; i++) {
        if (!dot_objs[i]) continue;
        bool active = (i == (int)np_state.current);
        lv_obj_set_width(dot_objs[i], active ? 18 : 5);
        lv_obj_set_style_bg_color(dot_objs[i],
            active ? NP_C_ACCENT : lv_color_hex(0x252A3C), 0);
    }
}

/* ================================================================
 * Navigation Rail — icon only, 44×44 square buttons
 * ================================================================ */
static void np_build_rail(void)
{
    np_rail = lv_obj_create(np_root);
    lv_obj_set_size(np_rail, NP_RAIL_W, NP_H);
    lv_obj_set_pos(np_rail, 0, 0);
    lv_obj_set_style_bg_color(np_rail,    NP_C_RAIL_BG, 0);
    lv_obj_set_style_bg_opa(np_rail,      LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(np_rail, 0, 0);
    lv_obj_set_style_radius(np_rail,       0, 0);
    lv_obj_set_style_pad_top(np_rail,      12, 0);
    lv_obj_set_style_pad_bottom(np_rail,   12, 0);
    lv_obj_set_style_pad_hor(np_rail,      0, 0);
    lv_obj_set_flex_flow(np_rail, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(np_rail,
        LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(np_rail, 2, 0);
    lv_obj_set_scrollbar_mode(np_rail, LV_SCROLLBAR_MODE_OFF);

    /* NP badge */
    lv_obj_t *logo = lv_obj_create(np_rail);
    lv_obj_set_size(logo, 36, 36);
    lv_obj_set_style_bg_color(logo,     NP_C_ACCENT, 0);
    lv_obj_set_style_radius(logo,       9, 0);
    lv_obj_set_style_border_width(logo, 0, 0);
    lv_obj_set_style_pad_all(logo,      0, 0);
    lv_obj_set_style_margin_bottom(logo, 6, 0);
    lv_obj_set_scrollbar_mode(logo, LV_SCROLLBAR_MODE_OFF);
    lv_obj_t *logo_l = lv_label_create(logo);
    lv_label_set_text(logo_l, "NP");
    lv_obj_set_style_text_font(logo_l,  NP_F_SM,     0);
    lv_obj_set_style_text_color(logo_l, NP_C_DARK_FG, 0);
    lv_obj_set_style_text_letter_space(logo_l, 1, 0);
    lv_obj_align(logo_l, LV_ALIGN_CENTER, 0, 0);

    /* separator */
    lv_obj_t *sep = lv_obj_create(np_rail);
    lv_obj_set_size(sep, 38, 1);
    lv_obj_set_style_bg_color(sep,     lv_color_hex(0x181B28), 0);
    lv_obj_set_style_bg_opa(sep,       LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(sep, 0, 0);
    lv_obj_set_style_radius(sep,       0, 0);
    lv_obj_set_style_pad_all(sep,      0, 0);
    lv_obj_set_style_margin_bottom(sep, 6, 0);

    /* ── Icon-only nav buttons ── */
    for (int i = 0; i < (int)NAV_N; i++) {
        lv_obj_t *btn = lv_button_create(np_rail);
        /* square: 44×44 — no label, just icon */
        lv_obj_set_size(btn, 44, 44);
        lv_obj_set_style_bg_color(btn, NP_C_RAIL_BG, 0);
        lv_obj_set_style_bg_color(btn, NP_C_ACCENT_BG, LV_STATE_CHECKED);
        lv_obj_set_style_border_color(btn, NP_C_BORDER,   LV_STATE_CHECKED);
        lv_obj_set_style_border_width(btn, 1,              LV_STATE_CHECKED);
        lv_obj_set_style_border_width(btn, 0, 0);
        lv_obj_set_style_radius(btn,       10, 0);
        lv_obj_set_style_shadow_width(btn, 0, 0);
        lv_obj_set_style_pad_all(btn,      0, 0);
        lv_obj_add_flag(btn, LV_OBJ_FLAG_CHECKABLE);

        lv_obj_t *ic = lv_label_create(btn);
        lv_label_set_text(ic, nav_items[i].sym);
        lv_obj_set_style_text_font(ic,  NP_F_XL, 0);
        lv_obj_set_style_text_color(ic, NP_C_TEXT_MUTED, 0);
        lv_obj_set_style_text_color(ic, NP_C_ACCENT, LV_STATE_CHECKED);
        lv_obj_align(ic, LV_ALIGN_CENTER, 0, 0);

        lv_obj_add_event_cb(btn, nav_cb, LV_EVENT_CLICKED,
                            (void *)(uintptr_t)nav_items[i].scr);
        rail_btn[nav_items[i].scr] = btn;
    }

    lv_obj_add_state(rail_btn[NP_SCR_HOME], LV_STATE_CHECKED);
}

/* ================================================================
 * Topbar — 60 px (bumped for 7" touch comfort)
 * ================================================================ */
static void np_build_topbar(void)
{
    np_topbar = lv_obj_create(np_root);
    lv_obj_set_size(np_topbar, NP_CONTENT_W, NP_TOPBAR_H);
    lv_obj_set_pos(np_topbar, NP_RAIL_W, 0);
    lv_obj_set_style_bg_color(np_topbar,    NP_C_RAIL_BG, 0);
    lv_obj_set_style_bg_opa(np_topbar,      LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(np_topbar, 0, 0);
    lv_obj_set_style_radius(np_topbar,       0, 0);
    lv_obj_set_style_pad_hor(np_topbar,      18, 0);
    lv_obj_set_style_pad_ver(np_topbar,      0, 0);
    lv_obj_set_flex_flow(np_topbar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(np_topbar,
        LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(np_topbar, 10, 0);
    lv_obj_set_scrollbar_mode(np_topbar, LV_SCROLLBAR_MODE_OFF);

    np_icon_btn(np_topbar, LV_SYMBOL_LIST);

    lv_obj_t *tb = lv_obj_create(np_topbar);
    lv_obj_set_height(tb, NP_TOPBAR_H);
    lv_obj_set_style_bg_opa(tb,     LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(tb, 0, 0);
    lv_obj_set_style_pad_all(tb,    0, 0);
    lv_obj_set_style_flex_grow(tb,  1, 0);
    lv_obj_set_flex_flow(tb, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(tb,
        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_scrollbar_mode(tb, LV_SCROLLBAR_MODE_OFF);

    topbar_title = np_label(tb, "Início", NP_F_LG, NP_C_TEXT);
    topbar_clock = np_label(tb, "quarta-feira, 18 de junho",
                            NP_F_XS, NP_C_TEXT_MUTED);

    /* online chip */
    lv_obj_t *chip = lv_obj_create(np_topbar);
    lv_obj_set_size(chip, LV_SIZE_CONTENT, 30);
    lv_obj_set_style_bg_color(chip,     lv_color_hex(0x0E1F14), 0);
    lv_obj_set_style_border_color(chip, NP_C_GREEN_BD, 0);
    lv_obj_set_style_border_width(chip, 1, 0);
    lv_obj_set_style_radius(chip,       15, 0);
    lv_obj_set_style_pad_hor(chip,      10, 0);
    lv_obj_set_style_pad_ver(chip,      0, 0);
    lv_obj_set_flex_flow(chip, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(chip,
        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(chip, 6, 0);
    lv_obj_set_scrollbar_mode(chip, LV_SCROLLBAR_MODE_OFF);

    lv_obj_t *dot = lv_obj_create(chip);
    lv_obj_set_size(dot, 6, 6);
    lv_obj_set_style_bg_color(dot,     NP_C_GREEN, 0);
    lv_obj_set_style_bg_opa(dot,       LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(dot, 0, 0);
    lv_obj_set_style_radius(dot,       3, 0);
    np_label(chip, "Online", NP_F_SM, NP_C_GREEN);

    lv_obj_t *bell = np_icon_btn(np_topbar, LV_SYMBOL_BELL);
    lv_obj_add_event_cb(bell, bell_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *vs = lv_obj_create(np_topbar);
    lv_obj_set_size(vs, 1, 18);
    lv_obj_set_style_bg_color(vs,     NP_C_BORDER, 0);
    lv_obj_set_style_bg_opa(vs,       LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(vs, 0, 0);
    lv_obj_set_style_radius(vs,       0, 0);

    np_icon_btn(np_topbar, LV_SYMBOL_TINT);

    lv_obj_t *gear = np_icon_btn(np_topbar, LV_SYMBOL_SETTINGS);
    lv_obj_add_event_cb(gear, gear_cb, LV_EVENT_CLICKED, NULL);
}

/* ================================================================
 * Dot strip — in normal flow below content (no overlap)
 * ================================================================ */
static void np_build_dots(lv_obj_t *col)
{
    np_dots_bar = lv_obj_create(col);
    lv_obj_set_size(np_dots_bar, lv_pct(100), NP_DOTS_H);
    np_obj_clear_style(np_dots_bar);
    lv_obj_set_flex_flow(np_dots_bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(np_dots_bar,
        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(np_dots_bar, 5, 0);

    for (int i = 0; i < NP_SCR_COUNT; i++) {
        lv_obj_t *d = lv_obj_create(np_dots_bar);
        bool active = (i == (int)NP_SCR_HOME);
        lv_obj_set_size(d, active ? 18 : 5, 5);
        lv_obj_set_style_bg_color(d,
            active ? NP_C_ACCENT : lv_color_hex(0x252A3C), 0);
        lv_obj_set_style_bg_opa(d,       LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(d, 0, 0);
        lv_obj_set_style_radius(d,       3, 0);
        dot_objs[i] = d;
    }
}

/* ================================================================
 * Navigation
 * ================================================================ */
void np_navigate_to(np_screen_t scr)
{
    if (np_screens[np_state.current])
        lv_obj_add_flag(np_screens[np_state.current], LV_OBJ_FLAG_HIDDEN);
    if (rail_btn[np_state.current])
        lv_obj_clear_state(rail_btn[np_state.current], LV_STATE_CHECKED);

    np_state.current = scr;

    if (np_screens[scr])
        lv_obj_clear_flag(np_screens[scr], LV_OBJ_FLAG_HIDDEN);
    if (rail_btn[scr])
        lv_obj_add_state(rail_btn[scr], LV_STATE_CHECKED);

    if (topbar_title)
        lv_label_set_text(topbar_title, scr_titles[scr]);

    dots_update();
}

/* ================================================================
 * Boot phase helpers
 * ================================================================ */
void np_boot_advance(uint8_t pct)
{
    if (np_state.phase != NP_PHASE_BOOT) return;
    np_state.boot_pct = pct;
    /* caller should update the boot progress bar widget */
}

void np_boot_finish(void)
{
    np_state.phase = NP_PHASE_SETUP;
    if (np_screens[NP_SCR_BOOT])
        lv_obj_add_flag(np_screens[NP_SCR_BOOT], LV_OBJ_FLAG_HIDDEN);
    if (np_screens[NP_SCR_SETUP])
        lv_obj_clear_flag(np_screens[NP_SCR_SETUP], LV_OBJ_FLAG_HIDDEN);
}

/* ================================================================
 * Setup phase helpers
 * ================================================================ */
void np_setup_next(void)
{
    if (np_state.setup_step < NP_SETUP_DONE) {
        np_state.setup_step++;
    } else {
        /* finish — hide setup, enter main */
        np_state.phase = NP_PHASE_MAIN;
        if (np_screens[NP_SCR_SETUP])
            lv_obj_add_flag(np_screens[NP_SCR_SETUP], LV_OBJ_FLAG_HIDDEN);
    }
}

void np_setup_back(void)
{
    if (np_state.setup_step > NP_SETUP_NAME)
        np_state.setup_step--;
}

/* ================================================================
 * Init
 * ================================================================ */
void np_init(void)
{
    np_root = lv_obj_create(lv_screen_active());
    lv_obj_set_size(np_root, NP_W, NP_H);
    lv_obj_set_pos(np_root, 0, 0);
    lv_obj_set_style_bg_color(np_root,    NP_C_BG, 0);
    lv_obj_set_style_bg_opa(np_root,      LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(np_root, 0, 0);
    lv_obj_set_style_radius(np_root,       0, 0);
    lv_obj_set_style_pad_all(np_root,      0, 0);
    lv_obj_set_scrollbar_mode(np_root,     LV_SCROLLBAR_MODE_OFF);

    np_build_rail();
    np_build_topbar();

    /* right column: content + dots */
    lv_obj_t *col = lv_obj_create(np_root);
    lv_obj_set_size(col, NP_CONTENT_W, NP_CONTENT_H);
    lv_obj_set_pos(col, NP_RAIL_W, NP_TOPBAR_H);
    np_obj_clear_style(col);
    lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(col,
        LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    /* content area — flex:1 */
    np_content = lv_obj_create(col);
    lv_obj_set_size(np_content, lv_pct(100),
                    NP_CONTENT_H - NP_DOTS_H);
    lv_obj_set_style_bg_opa(np_content,      LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(np_content, 0, 0);
    lv_obj_set_style_radius(np_content,       0, 0);
    lv_obj_set_style_pad_all(np_content,      NP_PAD, 0);
    lv_obj_set_scrollbar_mode(np_content,     LV_SCROLLBAR_MODE_OFF);

    /* dot strip — below content, in normal flow */
    np_build_dots(col);

    /* build main screens */
    np_screen_home(np_content);
    np_screen_agenda(np_content);
    np_screen_mercado(np_content);
    np_screen_casa(np_content);
    np_screen_alarmes(np_content);
    np_screen_clima(np_content);
    np_screen_timer(np_content);
    np_screen_config(np_content);
    np_screen_notif(np_content);

    /* hide all main screens initially */
    for (int i = 0; i < NP_SCR_COUNT; i++)
        if (np_screens[i]) lv_obj_add_flag(np_screens[i], LV_OBJ_FLAG_HIDDEN);

    /* overlay screens (boot + setup) on top of everything */
    np_screen_boot(np_root);
    np_screen_setup(np_root);

    /* hide setup — boot shows first */
    if (np_screens[NP_SCR_SETUP])
        lv_obj_add_flag(np_screens[NP_SCR_SETUP], LV_OBJ_FLAG_HIDDEN);
    /* boot is visible by default */
}

/* ================================================================
 * Tick — call every 1 s
 * ================================================================ */
void np_tick(void)
{
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    if (!t) return;

    if (topbar_clock && np_state.current == NP_SCR_HOME) {
        char buf[40];
        strftime(buf, sizeof(buf), "%A, %d de %B", t);
        lv_label_set_text(topbar_clock, buf);
    }

    if (np_state.timer_running && np_state.timer_sec > 0) {
        np_state.timer_sec--;
        if (np_state.timer_sec == 0)
            np_state.timer_running = false;
    }
}
