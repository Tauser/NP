/*
 * Alarmes -- arquetipo Lista. ASPIRACIONAL: nao ha servico de alarme
 * nem persistencia para ele.
 *
 * Mesmo padrao de linhas recicladas da Agenda. A diferenca e o toggle por
 * linha: ele muda apenas a cor de um retangulo de 44x24 dentro de uma
 * linha ja posicionada -- nenhuma reflow, nenhuma mudanca de geometria (R1).
 */

#include "np_components.h"
#include "np_screens.h"
#include "np_styles.h"

#define NP_ALARM_ROWS 5

static void alarm_toggle(lv_obj_t *parent, int32_t x, int32_t y, bool on)
{
    np_fill(parent, x, y, 44, 24, on ? np_c_accent() : np_c_hairline(), LV_OPA_COVER, 12);
    np_fill(parent, x + (on ? 22 : 2), y + 2, 20, 20,
            on ? np_c_text_on_accent() : np_c_text_3(), LV_OPA_COVER, LV_RADIUS_CIRCLE);
}

lv_obj_t *np_alarms_create(lv_obj_t *parent)
{
    lv_obj_t *root = np_scene(parent);

    np_label(root, "Alarmes", NP_FONT_TITLE, np_c_text(),
             NP_COL_X(0), NP_ROW_Y(0), 300, LV_TEXT_ALIGN_LEFT);
    np_label(root, "ASPIRACIONAL \xC2\xB7 sem servico de alarme", NP_FONT_SM,
             np_c_text_disabled(), NP_COL_X(6), NP_ROW_Y(0) + NP_SP_8, NP_COL_W(6),
             LV_TEXT_ALIGN_RIGHT);

    np_hline(root, NP_COL_X(0), NP_ROW_Y(1) + NP_SP_8, NP_W_FULL);

    static np_row_t rows[NP_ALARM_ROWS];
    const int32_t row_h = 72;
    const int32_t gap = NP_SP_12;

    static const char *hour[NP_ALARM_ROWS]  = { "06:30", "07:15", "12:00", "22:30", "" };
    static const char *title[NP_ALARM_ROWS] = {
        "Dias uteis", "Academia", "Remedio", "Dormir", ""
    };
    static const char *meta[NP_ALARM_ROWS]  = {
        "Seg a Sex", "Ter e Qui", "Todos os dias", "Todos os dias", ""
    };
    static const bool on[NP_ALARM_ROWS] = { true, false, true, true, false };

    for (int i = 0; i < NP_ALARM_ROWS; i++) {
        const int32_t y = NP_ROW_Y(2) - NP_SP_16 + i * (row_h + gap);
        rows[i] = np_row(root, NP_COL_X(0), y, NP_W_FULL, row_h);
        np_row_set(&rows[i], hour[i], title[i], meta[i],
                   on[i] ? np_c_accent() : np_c_text_disabled(), i < 4);
        if (i < 4) {
            alarm_toggle(rows[i].root, NP_W_FULL - NP_SP_24 - 44, (row_h - 24) / 2, on[i]);
        }
    }

    np_dots(root, NP_SCREEN_H - 20, 3, 1);
    return root;
}
