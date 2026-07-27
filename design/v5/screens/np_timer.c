/*
 * Timer -- arquetipo Hero. ASPIRACIONAL: nao ha servico de timer.
 *
 * O digito grande e o unico elemento que muda a cada segundo. Se esta
 * tela virar produto, ela e o pior caso de custo de render do sistema
 * (1 Hz contra os 0,017 Hz do relogio da Home), e por isso o contador usa
 * um label por digito -- sem isso, cada segundo invalida a caixa inteira
 * e busca todos os glifos em flash.
 */

#include "np_components.h"
#include "np_screens.h"
#include "np_styles.h"

lv_obj_t *np_timer_create(lv_obj_t *parent)
{
    lv_obj_t *root = np_scene(parent);

    np_label(root, "Timer", NP_FONT_SM, np_c_text_3(),
             NP_COL_X(0), NP_ROW_Y(0), NP_W_MAIN, LV_TEXT_ALIGN_LEFT);

    np_clock_t counter = np_clock(root, NP_COL_X(0), NP_ROW_Y(1), NP_FONT_HERO, np_c_text());
    np_clock_set(&counter, "04:35");

    np_label(root, "restante de 10:00", NP_FONT_LG, np_c_text_3(),
             NP_COL_X(0), NP_ROW_Y(3), NP_W_MAIN, LV_TEXT_ALIGN_LEFT);

    /* Progresso como retangulo chapado sobre trilho -- sem arc, que
     * forca draw layer e antialiasing caro a cada tique. */
    np_fill(root, NP_COL_X(0), NP_ROW_Y(4), NP_W_MAIN, 6, np_c_hairline(), LV_OPA_COVER, 3);
    np_fill(root, NP_COL_X(0), NP_ROW_Y(4), NP_W_MAIN * 55 / 100, 6, np_c_accent(), LV_OPA_COVER, 3);

    np_button(root, NP_COL_X(0), NP_ROW_Y(5) + NP_SP_16, 180, 52, "Pausar", true);
    np_button(root, NP_COL_X(0) + 200, NP_ROW_Y(5) + NP_SP_16, 180, 52, "Zerar", false);

    /* Predefinicoes. */
    lv_obj_t *card = np_surface(root, NP_X_ASIDE, NP_ROW_Y(0), NP_W_ASIDE, NP_ROW_H(7));
    const int32_t inner_w = NP_W_ASIDE - 2 * NP_SP_24;

    np_label(card, "Predefinicoes", NP_FONT_MD, np_c_text(), 0, 0, inner_w, LV_TEXT_ALIGN_LEFT);
    np_label(card, "ASPIRACIONAL \xC2\xB7 sem servico de timer", NP_FONT_SM, np_c_text_disabled(),
             0, NP_SP_24, inner_w, LV_TEXT_ALIGN_LEFT);

    static const char *presets[4] = { "1 min", "5 min", "10 min", "25 min" };
    const int32_t half = (inner_w - NP_SP_16) / 2;
    for (int i = 0; i < 4; i++) {
        np_button(card, (i % 2) * (half + NP_SP_16), 72 + (i / 2) * 72, half, 56, presets[i], false);
    }

    np_dots(root, NP_SCREEN_H - 20, 3, 2);
    return root;
}
