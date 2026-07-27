/*
 * Notificacoes -- arquetipo Lista. ASPIRACIONAL como tela navegavel.
 *
 * Atencao ao contrato do UI-PATTERN.md SS3: notificacao tem payload
 * individual e e um dos poucos casos com tratamento proprio no shell
 * (toast), NAO uma cena que se invalida por mascara. Esta tela e o
 * historico consultavel, nao o mecanismo de alerta.
 *
 * O componente de linha aqui e o mesmo da Agenda -- reciclado, altura
 * fixa, N criadas no build.
 */

#include "np_components.h"
#include "np_screens.h"
#include "np_styles.h"

#define NP_NOTIF_ROWS 6

lv_obj_t *np_notifications_create(lv_obj_t *parent)
{
    lv_obj_t *root = np_scene(parent);

    np_label(root, "Notificacoes", NP_FONT_TITLE, np_c_text(),
             NP_COL_X(0), NP_ROW_Y(0), 400, LV_TEXT_ALIGN_LEFT);
    np_label(root, "3 nao lidas", NP_FONT_SM, np_c_accent(),
             NP_COL_X(0), NP_ROW_Y(0) + NP_SP_32, 200, LV_TEXT_ALIGN_LEFT);
    np_button(root, NP_COL_X(9), NP_ROW_Y(0), NP_COL_W(3), 44, "Limpar tudo", false);

    np_hline(root, NP_COL_X(0), NP_ROW_Y(1) + NP_SP_8, NP_W_FULL);

    static np_row_t rows[NP_NOTIF_ROWS];
    const int32_t row_h = 62;
    const int32_t gap = NP_SP_12;

    for (int i = 0; i < NP_NOTIF_ROWS; i++) {
        rows[i] = np_row(root, NP_COL_X(0), NP_ROW_Y(2) - NP_SP_8 + i * (row_h + gap),
                         NP_W_FULL, row_h);
    }

    np_row_set(&rows[0], "14:32", "BTC subiu US$ 1.000", "Alerta de variacao",
               np_c_positive(), true);
    np_row_set(&rows[1], "13:05", "Wi-Fi reconectado", "Rede \xC2\xB7 ESP-Hosted",
               np_c_text_3(), true);
    np_row_set(&rows[2], "11:48", "Clima atualizado do cache", "Sem rede no momento",
               np_c_accent(), true);
    np_row_set(&rows[3], "09:12", "Display recuperado apos retry", "Sistema",
               np_c_negative(), true);
    np_row_set(&rows[4], "", "", "", np_c_text_3(), false);
    np_row_set(&rows[5], "", "", "", np_c_text_3(), false);

    np_dots(root, NP_SCREEN_H - 20, 3, 2);
    return root;
}
