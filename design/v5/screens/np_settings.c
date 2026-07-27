/*
 * Configuracoes -- arquetipo Grade.
 *
 * Configuracao e uma TAREFA NOMEADA, nao uma matriz longa de switches
 * (UX_LAYOUT_REDESIGN.md SS6, "controle por intencao"). Cada tile abre um
 * destino; diagnostico fica separado do uso diario para nao contaminar um
 * com o outro.
 *
 * As tarefas que ja existem no produto (rede, hora/fuso, brilho) estao em
 * cor normal; as que ainda nao existem, em cinza-desabilitado.
 */

#include "np_components.h"
#include "np_screens.h"
#include "np_styles.h"

lv_obj_t *np_settings_create(lv_obj_t *parent)
{
    lv_obj_t *root = np_scene(parent);

    np_label(root, "Configuracoes", NP_FONT_TITLE, np_c_text(),
             NP_COL_X(0), NP_ROW_Y(0), 400, LV_TEXT_ALIGN_LEFT);

    np_hline(root, NP_COL_X(0), NP_ROW_Y(1), NP_W_FULL);

    static np_tile_t tiles[6];
    static const char *icon[6] = { "W", "T", "B", "N", "S", "I" };
    static const char *name[6] = {
        "Wi-Fi e rede", "Hora e fuso", "Brilho",
        "Notificacoes", "Sistema", "Sobre"
    };
    static const char *state[6] = {
        "Conectado", "America/Sao_Paulo \xC2\xB7 24h", "80%",
        "Em breve", "Diagnostico", "v4 \xC2\xB7 esp32p4"
    };
    /* Tarefa ja existente no firmware? */
    static const bool live[6] = { true, true, true, false, false, true };

    const int32_t tile_w = NP_COL_W(4);
    const int32_t tile_h = NP_ROW_H(2) + NP_SP_16;

    for (int i = 0; i < 6; i++) {
        const int32_t x = NP_COL_X((i % 3) * 4);
        const int32_t y = NP_ROW_Y(2) - NP_SP_16 + (i / 3) * (tile_h + NP_SP_24);
        tiles[i] = np_tile(root, x, y, tile_w, tile_h, icon[i], name[i], state[i]);
        if (!live[i]) {
            np_set_text_color(tiles[i].title, np_c_text_disabled());
            np_set_text_color(tiles[i].icon, np_c_text_disabled());
        }
    }

    np_dots(root, NP_SCREEN_H - 20, 3, 1);
    return root;
}
