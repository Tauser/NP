/*
 * Clima -- arquetipo Hero. ASPIRACIONAL em quase tudo.
 *
 * Hoje o estado guarda apenas temperatura, precipitacao e um resumo
 * curto. Vento, umidade, sensacao e previsao por periodo NAO tem
 * provider: o Open-Meteo so entrega esses campos em blocos hourly/daily,
 * que estourariam o orcamento de payload da SS5 do RESOURCE-BUDGET.
 *
 * O que aparece aqui em cinza-terciario e exatamente o que ainda nao tem
 * fonte -- a intencao e deixar visivel o custo de "completar" a tela.
 */

#include "np_components.h"
#include "np_screens.h"
#include "np_styles.h"

lv_obj_t *np_weather_create(lv_obj_t *parent)
{
    lv_obj_t *root = np_scene(parent);

    np_label(root, "Clima", NP_FONT_SM, np_c_text_3(),
             NP_COL_X(0), NP_ROW_Y(0), NP_W_MAIN, LV_TEXT_ALIGN_LEFT);

    /* Real: vem do Open-Meteo hoje. */
    np_label(root, "24\xC2\xB0", NP_FONT_HERO, np_c_text(),
             NP_COL_X(0), NP_ROW_Y(0) + NP_SP_24, 260, LV_TEXT_ALIGN_LEFT);
    np_label(root, "Parcialmente nublado", NP_FONT_TITLE, np_c_text_2(),
             NP_COL_X(0), NP_ROW_Y(2), NP_W_MAIN, LV_TEXT_ALIGN_LEFT);
    np_label(root, "Chuva 1,5 mm", NP_FONT_MD, np_c_text_2(),
             NP_COL_X(0), NP_ROW_Y(2) + NP_SP_48, NP_W_MAIN, LV_TEXT_ALIGN_LEFT);

    np_hline(root, NP_COL_X(0), NP_ROW_Y(4), NP_W_MAIN);

    /* Aspiracional: sem provider. */
    np_label(root, "SEM FONTE DE DADO HOJE", NP_FONT_SM, np_c_text_disabled(),
             NP_COL_X(0), NP_ROW_Y(4) + NP_SP_16, NP_W_MAIN, LV_TEXT_ALIGN_LEFT);

    const int32_t third = (NP_W_MAIN - 2 * NP_SP_24) / 3;
    const int32_t y = NP_ROW_Y(5);
    np_metric(root, NP_COL_X(0), y, third, "Sensacao", "--", NP_FONT_LG);
    np_metric(root, NP_COL_X(0) + third + NP_SP_24, y, third, "Umidade", "--", NP_FONT_LG);
    np_metric(root, NP_COL_X(0) + 2 * (third + NP_SP_24), y, third, "Vento", "--", NP_FONT_LG);

    /* Previsao por periodo -- tambem sem fonte. */
    lv_obj_t *card = np_surface(root, NP_X_ASIDE, NP_ROW_Y(0), NP_W_ASIDE, NP_ROW_H(7));
    const int32_t inner_w = NP_W_ASIDE - 2 * NP_SP_24;

    np_label(card, "Proximos periodos", NP_FONT_MD, np_c_text(), 0, 0, inner_w, LV_TEXT_ALIGN_LEFT);
    np_label(card, "Requer bloco hourly do Open-Meteo", NP_FONT_SM, np_c_text_disabled(),
             0, NP_SP_24, inner_w, LV_TEXT_ALIGN_LEFT);

    static const char *slots[4] = { "Manha", "Tarde", "Noite", "Madrugada" };
    for (int i = 0; i < 4; i++) {
        const int32_t row_y = 80 + i * 72;
        np_label(card, slots[i], NP_FONT_MD, np_c_text_2(), 0, row_y, 160, LV_TEXT_ALIGN_LEFT);
        np_label(card, "--", NP_FONT_LG, np_c_text_disabled(),
                 inner_w - 100, row_y - 4, 100, LV_TEXT_ALIGN_RIGHT);
        np_hline(card, 0, row_y + 48, inner_w);
    }

    np_dots(root, NP_SCREEN_H - 20, 3, 2);
    return root;
}
