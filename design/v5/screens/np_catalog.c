/*
 * Catalogo das telas -- ponto unico que o harness de mockup itera.
 *
 * A ordem aqui e a ordem em que os mockups sao gerados e revisados.
 */

#include "np_screens.h"

static const np_screen_entry_t s_catalog[] = {
    /* implementadas no firmware */
    { "boot",          "Boot",           "Fluxo", np_boot_create },
    { "home",          "Agora",          "Hero",  np_home_create },
    { "market",        "Mercado",        "Hero",  np_market_create },
    { "setup",         "Setup",          "Fluxo", np_setup_create },
    /* aspiracionais */
    { "weather",       "Clima",          "Hero",  np_weather_create },
    { "timer",         "Timer",          "Hero",  np_timer_create },
    { "agenda",        "Agenda",         "Lista", np_agenda_create },
    { "alarms",        "Alarmes",        "Lista", np_alarms_create },
    { "notifications", "Notificacoes",   "Lista", np_notifications_create },
    { "devices",       "Casa",           "Grade", np_devices_create },
    { "settings",      "Configuracoes",  "Grade", np_settings_create },
    { "sheets",        "Painel lateral", "Fluxo", np_sheets_create },
};

const np_screen_entry_t *np_screen_catalog(void)
{
    return s_catalog;
}

int np_screen_catalog_count(void)
{
    return (int)(sizeof(s_catalog) / sizeof(s_catalog[0]));
}
