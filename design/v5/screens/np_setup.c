/*
 * Setup -- arquetipo Fluxo. Full-bleed, um passo por vez.
 *
 * Wifi -> Fuso/Formato -> Confirmacao. Os tres grupos sao criados no
 * build e trocados por LV_OBJ_FLAG_HIDDEN, NUNCA por rebuild: reconstruir
 * a arvore a cada passo joga fora o trabalho de layout e produz um pico
 * de alocacao em SRAM interna bem no meio do onboarding.
 *
 * Este modelo mostra o passo 1. As linhas de rede sao recicladas (mesmo
 * componente da Agenda), entao um scan que devolva 30 redes nao aloca 30
 * arvores de widget.
 */

#include "np_components.h"
#include "np_screens.h"
#include "np_styles.h"

#define NP_WIFI_ROWS 5

lv_obj_t *np_setup_create(lv_obj_t *parent)
{
    lv_obj_t *root = np_scene(parent);

    np_label(root, "Passo 1 de 3", NP_FONT_SM, np_c_accent(),
             NP_COL_X(2), NP_ROW_Y(0) + NP_SP_16, NP_COL_W(8), LV_TEXT_ALIGN_LEFT);
    np_label(root, "Escolha a rede Wi-Fi", NP_FONT_TITLE, np_c_text(),
             NP_COL_X(2), NP_ROW_Y(0) + NP_SP_48, NP_COL_W(8), LV_TEXT_ALIGN_LEFT);

    np_segbar_t bar = np_segbar(root, NP_COL_X(2), NP_ROW_Y(1) + NP_SP_32, NP_COL_W(8), 3);
    np_segbar_set(&bar, 1, np_c_accent());

    np_label(root, "Transporte ativo \xC2\xB7 4 redes encontradas", NP_FONT_SM, np_c_text_3(),
             NP_COL_X(2), NP_ROW_Y(2) - NP_SP_8, NP_COL_W(8), LV_TEXT_ALIGN_LEFT);

    static np_row_t rows[NP_WIFI_ROWS];
    const int32_t row_h = 56;
    const int32_t gap = NP_SP_8;

    static const char *lead[NP_WIFI_ROWS]  = { ")))", ")))", "))", ")", "" };
    static const char *ssid[NP_WIFI_ROWS]  = {
        "NovaNet", "NovaNet_5G", "Vizinho-2G", "Convidados", ""
    };
    static const char *meta[NP_WIFI_ROWS]  = {
        "Protegida \xC2\xB7 -42 dBm", "Protegida \xC2\xB7 -51 dBm",
        "Protegida \xC2\xB7 -68 dBm", "Aberta \xC2\xB7 -74 dBm", ""
    };

    for (int i = 0; i < NP_WIFI_ROWS; i++) {
        const int32_t y = NP_ROW_Y(2) + NP_SP_24 + i * (row_h + gap);
        rows[i] = np_row(root, NP_COL_X(2), y, NP_COL_W(8), row_h);
        np_row_set(&rows[i], lead[i], ssid[i], meta[i],
                   i == 0 ? np_c_accent() : np_c_text_3(), i < 4);
    }

    np_button(root, NP_COL_X(2), NP_ROW_Y(7), 180, 52, "Escanear", false);
    np_button(root, NP_COL_X(8), NP_ROW_Y(7), NP_COL_W(4), 52, "Avancar", true);

    return root;
}
