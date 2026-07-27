/*
 * Home ("Agora") -- arquetipo Hero.
 *
 *   col 0-6   relogio herói, data, clima
 *   col 7-11  BTC em foco, com aura de direcao e sparkline
 *
 * O relogio e o elemento dominante e usa um label por digito (R8): a
 * tela muda uma vez por MINUTO, ainda que o evento de relogio chegue a
 * cada segundo.
 *
 * A aura verde/vermelha e um retangulo chapado em opacidade baixa, nunca
 * uma sombra (R5).
 *
 * Sem bloco de agenda: nao existe servico de calendario. O espaco foi
 * para o BTC em vez de virar moldura de dado inexistente.
 */

#include "np_components.h"
#include "np_screens.h"
#include "np_styles.h"

lv_obj_t *np_home_create(lv_obj_t *parent)
{
    lv_obj_t *root = np_scene(parent);

    /* ---------------- coluna esquerda ---------------- */

    np_clock_t clock = np_clock(root, NP_COL_X(0), NP_ROW_Y(0) + NP_SP_16, NP_FONT_HERO, np_c_text());
    np_clock_set(&clock, "14:30");

    np_label(root, "Quinta-feira, 24 de julho", NP_FONT_LG, np_c_text_3(),
             NP_COL_X(0), NP_ROW_Y(1) + NP_SP_24, NP_W_HALF, LV_TEXT_ALIGN_LEFT);

    np_hline(root, NP_COL_X(0), NP_ROW_Y(2) + NP_SP_16, NP_W_HALF);

    /* Clima: temperatura carrega o peso visual. Sem ilustracao -- os
     * assets por condicao ainda nao existem e um sol fixo mentiria
     * quando estivesse chovendo. */
    np_label(root, "Clima agora", NP_FONT_SM, np_c_text_3(),
             NP_COL_X(0), NP_ROW_Y(3), NP_W_HALF, LV_TEXT_ALIGN_LEFT);
    np_label(root, "24\xC2\xB0", NP_FONT_DISPLAY, np_c_text(),
             NP_COL_X(0), NP_ROW_Y(3) + NP_SP_24, 220, LV_TEXT_ALIGN_LEFT);
    np_label(root, "Parcialmente nublado", NP_FONT_MD, np_c_text_2(),
             NP_COL_X(0), NP_ROW_Y(4) + NP_SP_24, NP_W_HALF, LV_TEXT_ALIGN_LEFT);

    /* Rodape: dolar como cotacao secundaria. */
    np_hline(root, NP_COL_X(0), NP_ROW_Y(6), NP_W_HALF);
    np_metric(root, NP_COL_X(0), NP_ROW_Y(6) + NP_SP_16, 200, "Dolar", "R$ 5,43", NP_FONT_LG);
    np_label(root, "-0,25%", NP_FONT_SM, np_c_negative(),
             NP_COL_X(0) + 210, NP_ROW_Y(6) + NP_SP_16 + NP_SP_24, 120, LV_TEXT_ALIGN_LEFT);

    /* ---------------- coluna direita: BTC ---------------- */

    lv_obj_t *card = np_surface(root, NP_X_ASIDE, NP_ROW_Y(0), NP_W_ASIDE, NP_ROW_H(7));
    const int32_t inner_w = NP_W_ASIDE - 2 * NP_SP_24;

    /* Aura de direcao: retangulo chapado, opacidade baixa (R5). */
    np_fill(card, -NP_SP_24, -NP_SP_24, NP_W_ASIDE, 140,
            np_c_positive(), LV_OPA_10, NP_RADIUS_SURFACE);

    np_label(card, "Bitcoin", NP_FONT_MD, np_c_text(), 0, 0, 140, LV_TEXT_ALIGN_LEFT);
    np_pill_t pill = np_status_pill(card, inner_w - 120, 2, NP_DATA_LIVE);
    (void)pill;

    np_label(card, "US$ 68.001", NP_FONT_DISPLAY, np_c_text(),
             0, NP_SP_32, inner_w, LV_TEXT_ALIGN_LEFT);
    np_label(card, "+1,25%", NP_FONT_TITLE, np_c_positive(),
             0, NP_SP_32 + 56, 160, LV_TEXT_ALIGN_LEFT);
    np_label(card, "em 24 h", NP_FONT_SM, np_c_text_3(),
             170, NP_SP_32 + 68, 120, LV_TEXT_ALIGN_LEFT);

    /* Sparkline: indicador de direcao a partir de amostras reais. */
    static const int32_t samples[12] = {
        66900, 67120, 67040, 67380, 67260, 67610,
        67480, 67720, 67900, 67810, 68050, 68001
    };
    np_spark_t spark = np_spark(card, 0, 200, inner_w, 96);
    np_spark_set(&spark, samples, 12, inner_w, 96, np_c_positive());

    np_label(card, "Ultimas 12 amostras", NP_FONT_SM, np_c_text_3(),
             0, 306, inner_w, LV_TEXT_ALIGN_LEFT);

    np_hline(card, 0, 340, inner_w);

    np_metric(card, 0, 356, 150, "Abertura", "US$ 67.161", NP_FONT_MD);
    np_metric(card, 170, 356, 150, "Maxima", "US$ 68.900", NP_FONT_MD);
    np_metric(card, 0, 412, 150, "Minima", "US$ 66.500", NP_FONT_MD);
    np_metric(card, 170, 412, 150, "Volume", "US$ 1,23 bi", NP_FONT_MD);

    np_dots(root, NP_SCREEN_H - 20, 3, 0);
    return root;
}
