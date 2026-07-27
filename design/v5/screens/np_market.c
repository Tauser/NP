/*
 * Mercado -- arquetipo Hero. O ativo e o protagonista; as metricas de 24 h
 * sao contexto, nao quatro cards independentes (UX_LAYOUT_REDESIGN.md
 * SS5.2).
 *
 * Cache e indisponibilidade entram no MESMO lugar do selo de estado, sem
 * reorganizar a tela -- e o que mantem a geometria congelada (R1).
 */

#include "np_components.h"
#include "np_screens.h"
#include "np_styles.h"

lv_obj_t *np_market_create(lv_obj_t *parent)
{
    lv_obj_t *root = np_scene(parent);

    /* ---------------- BTC em foco ---------------- */

    np_label(root, "Bitcoin \xC2\xB7 BTC/USD", NP_FONT_SM, np_c_text_3(),
             NP_COL_X(0), NP_ROW_Y(0), NP_W_MAIN - 140, LV_TEXT_ALIGN_LEFT);
    np_pill_t btc_pill = np_status_pill(root, NP_COL_X(0) + NP_W_MAIN - 130, NP_ROW_Y(0),
                                        NP_DATA_LIVE);
    (void)btc_pill;

    np_label(root, "US$ 68.001", NP_FONT_HERO, np_c_text(),
             NP_COL_X(0), NP_ROW_Y(0) + NP_SP_24, NP_W_MAIN, LV_TEXT_ALIGN_LEFT);
    np_label(root, "+1,25% nas ultimas 24 horas", NP_FONT_LG, np_c_positive(),
             NP_COL_X(0), NP_ROW_Y(1) + NP_SP_32, NP_W_MAIN, LV_TEXT_ALIGN_LEFT);

    static const int32_t samples[16] = {
        66500, 66780, 66640, 67010, 66890, 67240, 67120, 67460,
        67330, 67680, 67540, 67810, 67960, 67880, 68120, 68001
    };
    np_spark_t spark = np_spark(root, NP_COL_X(0), NP_ROW_Y(2) + NP_SP_24, NP_W_MAIN, 120);
    np_spark_set(&spark, samples, 16, NP_W_MAIN, 120, np_c_positive());

    np_hline(root, NP_COL_X(0), NP_ROW_Y(4) + NP_SP_16, NP_W_MAIN);

    np_label(root, "RESUMO DE 24 HORAS", NP_FONT_SM, np_c_text_3(),
             NP_COL_X(0), NP_ROW_Y(4) + NP_SP_32, NP_W_MAIN, LV_TEXT_ALIGN_LEFT);

    const int32_t quarter = (NP_W_MAIN - 3 * NP_SP_24) / 4;
    const int32_t metrics_y = NP_ROW_Y(5) + NP_SP_16;
    np_metric(root, NP_COL_X(0), metrics_y, quarter, "Abertura", "US$ 67.161", NP_FONT_MD);
    np_metric(root, NP_COL_X(0) + (quarter + NP_SP_24), metrics_y, quarter,
              "Maxima", "US$ 68.900", NP_FONT_MD);
    np_metric(root, NP_COL_X(0) + 2 * (quarter + NP_SP_24), metrics_y, quarter,
              "Minima", "US$ 66.500", NP_FONT_MD);
    np_metric(root, NP_COL_X(0) + 3 * (quarter + NP_SP_24), metrics_y, quarter,
              "Volume", "US$ 1,23 bi", NP_FONT_MD);

    /* ---------------- USD/BRL secundario ---------------- */

    lv_obj_t *card = np_surface(root, NP_X_ASIDE, NP_ROW_Y(0), NP_W_ASIDE, NP_ROW_H(7));
    const int32_t inner_w = NP_W_ASIDE - 2 * NP_SP_24;

    np_label(card, "Dolar \xC2\xB7 USD/BRL", NP_FONT_SM, np_c_text_3(),
             0, 0, inner_w - 120, LV_TEXT_ALIGN_LEFT);
    np_pill_t fx_pill = np_status_pill(card, inner_w - 120, 0, NP_DATA_STALE);
    (void)fx_pill;

    np_label(card, "R$ 5,43", NP_FONT_DISPLAY, np_c_text(),
             0, NP_SP_32, inner_w, LV_TEXT_ALIGN_LEFT);
    np_label(card, "-0,25% \xC2\xB7 24h", NP_FONT_LG, np_c_negative(),
             0, NP_SP_32 + 56, inner_w, LV_TEXT_ALIGN_LEFT);

    np_hline(card, 0, 168, inner_w);

    const int32_t half = (inner_w - NP_SP_24) / 2;
    np_metric(card, 0, 188, half, "Maxima", "R$ 5,45", NP_FONT_MD);
    np_metric(card, half + NP_SP_24, 188, half, "Minima", "R$ 5,40", NP_FONT_MD);

    np_hline(card, 0, 268, inner_w);
    np_label(card, "Atualizado ha 12 min", NP_FONT_SM, np_c_text_3(),
             0, 288, inner_w, LV_TEXT_ALIGN_LEFT);

    np_dots(root, NP_SCREEN_H - 20, 3, 1);
    return root;
}
