/*
 * Casa/Dispositivos -- arquetipo Grade. ASPIRACIONAL: automacao e fase
 * futura do ROADMAP, fora do MVP.
 *
 * Tiles sao ESTATICOS: posicao e tamanho congelados no build, so o
 * rotulo de estado e a cor mudam quando um dispositivo liga/desliga. E a
 * regra de atualizacao do arquetipo Grade -- nenhuma reorganizacao de
 * grade em runtime.
 */

#include "np_components.h"
#include "np_screens.h"
#include "np_styles.h"

lv_obj_t *np_devices_create(lv_obj_t *parent)
{
    lv_obj_t *root = np_scene(parent);

    np_label(root, "Casa", NP_FONT_TITLE, np_c_text(),
             NP_COL_X(0), NP_ROW_Y(0), 300, LV_TEXT_ALIGN_LEFT);
    np_label(root, "ASPIRACIONAL \xC2\xB7 automacao e fase futura", NP_FONT_SM,
             np_c_text_disabled(), NP_COL_X(6), NP_ROW_Y(0) + NP_SP_8, NP_COL_W(6),
             LV_TEXT_ALIGN_RIGHT);

    np_hline(root, NP_COL_X(0), NP_ROW_Y(1), NP_W_FULL);

    /* Grade 3 x 2 de tiles sobre as 12 colunas. */
    static np_tile_t tiles[6];
    static const char *icon[6]  = { "*", "~", "#", "@", "+", "=" };
    static const char *name[6]  = {
        "Luz da sala", "Ar-condicionado", "Tomada da TV",
        "Luz do quarto", "Umidificador", "Cafeteira"
    };
    static const char *state[6] = { "Ligada", "23\xC2\xB0 \xC2\xB7 frio", "Desligada",
                                    "Desligada", "Ligado", "Desligada" };
    static const bool on[6] = { true, true, false, false, true, false };

    const int32_t tile_w = NP_COL_W(4);
    const int32_t tile_h = NP_ROW_H(2) + NP_SP_16;

    for (int i = 0; i < 6; i++) {
        const int32_t x = NP_COL_X((i % 3) * 4);
        const int32_t y = NP_ROW_Y(2) - NP_SP_16 + (i / 3) * (tile_h + NP_SP_24);
        tiles[i] = np_tile(root, x, y, tile_w, tile_h, icon[i], name[i], state[i]);
        np_tile_set_on(&tiles[i], on[i], state[i]);
    }

    np_dots(root, NP_SCREEN_H - 20, 3, 0);
    return root;
}
