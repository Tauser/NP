/*
 * Boot -- arquetipo Fluxo. Full-bleed, sem chrome, um passo por vez.
 *
 * Cinco estagios derivados de estado real (display, storage, rede,
 * relogio, dados). Sem spinner e sem animacao continua: o boot util dura
 * ~1,5 s e uma animacao ai so compete por banda de MSPI justamente
 * enquanto o resto do sistema esta subindo.
 */

#include "np_components.h"
#include "np_screens.h"
#include "np_styles.h"

lv_obj_t *np_boot_create(lv_obj_t *parent)
{
    lv_obj_t *root = np_scene(parent);

    /* Marca de acento a esquerda do wordmark: um retangulo, nao um logo
     * bitmap -- 0 B de asset. */
    np_fill(root, NP_COL_X(3), NP_ROW_Y(2) + 14, 6, 44, np_c_accent(), LV_OPA_COVER, 3);

    np_label(root, "Nova", NP_FONT_HERO, np_c_text(),
             NP_COL_X(3) + NP_SP_24, NP_ROW_Y(2), 180, LV_TEXT_ALIGN_LEFT);
    np_label(root, "Panel", NP_FONT_HERO, np_c_accent(),
             NP_COL_X(3) + NP_SP_24 + 132, NP_ROW_Y(2), 200, LV_TEXT_ALIGN_LEFT);

    np_hline(root, NP_COL_X(3), NP_ROW_Y(4), NP_COL_W(6));

    /* Estagios: preenchidos conforme o boot avanca. */
    np_segbar_t bar = np_segbar(root, NP_COL_X(3), NP_ROW_Y(4) + NP_SP_32, NP_COL_W(6), 5);
    np_segbar_set(&bar, 3, np_c_accent());

    np_label(root, "Sincronizando o relogio", NP_FONT_LG, np_c_text_2(),
             NP_COL_X(3), NP_ROW_Y(5), NP_COL_W(6), LV_TEXT_ALIGN_LEFT);
    np_label(root, "O painel abre com os ultimos dados salvos se a rede demorar",
             NP_FONT_SM, np_c_text_3(),
             NP_COL_X(3), NP_ROW_Y(5) + NP_SP_32, NP_COL_W(6), LV_TEXT_ALIGN_LEFT);

    np_label(root, "v4 \xC2\xB7 esp32p4", NP_FONT_SM, np_c_text_disabled(),
             NP_COL_X(3), NP_ROW_Y(7) + NP_SP_16, NP_COL_W(6), LV_TEXT_ALIGN_LEFT);

    return root;
}
