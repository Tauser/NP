/* ================================================================
 * NOVAPANEL — Main Header  (v2 — boot + setup + icon-only rail)
 * LVGL v9.5  |  ESP32-P4  |  1024×600
 * ================================================================ */
#ifndef NOVAPANEL_H
#define NOVAPANEL_H

#include "lvgl.h"
#include "novapanel_theme.h"

/* ── App phase ────────────────────────────────────────────────── */
typedef enum {
    NP_PHASE_BOOT  = 0,   /* animated splash               */
    NP_PHASE_SETUP,       /* first-run setup wizard        */
    NP_PHASE_MAIN,        /* normal operation              */
} np_phase_t;

/* ── Screens ──────────────────────────────────────────────────── */
typedef enum {
    NP_SCR_HOME = 0,
    NP_SCR_AGENDA,
    NP_SCR_MERCADO,
    NP_SCR_CASA,
    NP_SCR_ALARMES,
    NP_SCR_CLIMA,
    NP_SCR_TIMER,
    NP_SCR_CONFIG,
    NP_SCR_NOTIF,
    NP_SCR_COUNT,
    /* overlay phases — not in nav rail */
    NP_SCR_BOOT  = NP_SCR_COUNT,
    NP_SCR_SETUP,
    NP_SCR_TOTAL
} np_screen_t;

/* ── Setup wizard step ────────────────────────────────────────── */
typedef enum {
    NP_SETUP_NAME = 0,
    NP_SETUP_WIFI,
    NP_SETUP_TZ,
    NP_SETUP_DONE,
    NP_SETUP_STEPS
} np_setup_step_t;

/* ── App state ────────────────────────────────────────────────── */
typedef struct {
    /* phase */
    np_phase_t      phase;
    uint8_t         boot_pct;         /* 0-100 progress             */

    /* setup wizard */
    np_setup_step_t setup_step;
    char            user_name[32];
    char            wifi_ssid[32];
    char            wifi_pass[64];
    char            setup_tz[48];
    bool            time_fmt_24h;

    /* main app */
    np_screen_t     current;
    bool            night_mode;
    bool            bt_enabled;
    int32_t         brilho;
    int32_t         vol_system;
    int32_t         vol_music;
    int32_t         vol_alarm;
    int32_t         timer_sec;
    int32_t         timer_total;
    bool            timer_running;
    bool            dev_luz_sala;
    bool            dev_luz_cozinha;
    bool            dev_luz_quarto;
    bool            dev_tomada_tv;
    bool            dev_tomada_cafe;
    bool            dev_ventilador;
    bool            alarm_on[4];
} np_state_t;

extern np_state_t np_state;

/* ── Root objects ─────────────────────────────────────────────── */
extern lv_obj_t *np_root;
extern lv_obj_t *np_rail;
extern lv_obj_t *np_topbar;
extern lv_obj_t *np_content;           /* screens container  */
extern lv_obj_t *np_dots_bar;          /* swipe-indicator row */
extern lv_obj_t *np_screens[NP_SCR_TOTAL];

/* ── Core API ─────────────────────────────────────────────────── */
void np_init(void);
void np_navigate_to(np_screen_t screen);
void np_tick(void);                    /* call every 1 s         */
void np_boot_advance(uint8_t pct);     /* call from periodic task */
void np_boot_finish(void);             /* jump to setup immediately */
void np_setup_next(void);
void np_setup_back(void);

/* ── Screen builders ──────────────────────────────────────────── */
void np_screen_home(lv_obj_t *parent);
void np_screen_agenda(lv_obj_t *parent);
void np_screen_mercado(lv_obj_t *parent);
void np_screen_casa(lv_obj_t *parent);
void np_screen_alarmes(lv_obj_t *parent);
void np_screen_clima(lv_obj_t *parent);
void np_screen_timer(lv_obj_t *parent);
void np_screen_config(lv_obj_t *parent);
void np_screen_notif(lv_obj_t *parent);
void np_screen_boot(lv_obj_t *parent);
void np_screen_setup(lv_obj_t *parent);

#endif /* NOVAPANEL_H */
