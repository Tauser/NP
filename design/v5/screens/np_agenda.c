/*
 * Agenda -- arquetipo Lista. ASPIRACIONAL: nao existe servico de
 * calendario (ROADMAP Fase 8).
 *
 * O ponto desta tela como referencia e o padrao de LINHAS RECICLADAS: o
 * viewport cria N linhas de altura fixa no build e reusa; rolar ou
 * atualizar apenas re-texta. Criar um objeto por item faria o consumo de
 * SRAM interna virar funcao do volume de dados -- o jeito mais facil de
 * estourar o orcamento sem ninguem perceber.
 */

#include "np_components.h"
#include "np_screens.h"
#include "np_styles.h"

#define NP_AGENDA_ROWS 6

lv_obj_t *np_agenda_create(lv_obj_t *parent)
{
    lv_obj_t *root = np_scene(parent);

    np_label(root, "Agenda", NP_FONT_TITLE, np_c_text(),
             NP_COL_X(0), NP_ROW_Y(0), 300, LV_TEXT_ALIGN_LEFT);
    np_label(root, "Quinta-feira, 24 de julho", NP_FONT_SM, np_c_text_3(),
             NP_COL_X(0), NP_ROW_Y(0) + NP_SP_32, 400, LV_TEXT_ALIGN_LEFT);
    np_label(root, "ASPIRACIONAL \xC2\xB7 sem servico de calendario", NP_FONT_SM,
             np_c_text_disabled(), NP_COL_X(7), NP_ROW_Y(0) + NP_SP_32, NP_COL_W(5),
             LV_TEXT_ALIGN_RIGHT);

    np_hline(root, NP_COL_X(0), NP_ROW_Y(1) + NP_SP_8, NP_W_FULL);

    /* Viewport de altura fixa: N linhas criadas uma vez. */
    static np_row_t rows[NP_AGENDA_ROWS];
    const int32_t row_h = 62;
    const int32_t gap = NP_SP_12;

    for (int i = 0; i < NP_AGENDA_ROWS; i++) {
        rows[i] = np_row(root, NP_COL_X(0), NP_ROW_Y(2) - NP_SP_8 + i * (row_h + gap),
                         NP_W_FULL, row_h);
    }

    np_row_set(&rows[0], "09:00", "Revisao de firmware", "Sala 2 \xC2\xB7 45 min",
               np_c_accent(), true);
    np_row_set(&rows[1], "11:30", "Almoco com a equipe", "Fora do escritorio",
               np_c_text_3(), true);
    np_row_set(&rows[2], "14:00", "Bancada: soak de 72 h", "Laboratorio",
               np_c_positive(), true);
    np_row_set(&rows[3], "15:30", "Reuniao NoiseBot", "Chamada \xC2\xB7 30 min",
               np_c_accent(), true);
    np_row_set(&rows[4], "17:00", "Retrospectiva", "Sala 1",
               np_c_text_3(), true);
    /* Linha existente, sem dado: escondida, nao destruida. */
    np_row_set(&rows[5], "", "", "", np_c_text_3(), false);

    np_dots(root, NP_SCREEN_H - 20, 3, 0);
    return root;
}
