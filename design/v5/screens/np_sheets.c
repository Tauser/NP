/*
 * Painel lateral (sheet) -- arquetipo Fluxo, sobreposto.
 *
 * Cobre os paineis de Wi-Fi/perfil/detalhe rapido. E uma SOBREPOSICAO
 * TEMPORARIA, nunca uma cena de navegacao (UX_LAYOUT_REDESIGN.md SS4.2,
 * camada 3): nao entra no seletor de cenas e nao tem posicao nos dots.
 *
 * Custo: o scrim escurece a tela inteira, entao abrir/fechar invalida
 * 1024x600 -- e a operacao mais cara do sistema depois de trocar de cena.
 * Por isso o scrim e um retangulo chapado em opacidade fixa e nao um
 * blur/gradiente, e por isso o painel nao anima largura (animar largura
 * repintaria a area inteira a cada frame).
 */

#include "np_components.h"
#include "np_screens.h"
#include "np_styles.h"

lv_obj_t *np_sheets_create(lv_obj_t *parent)
{
    lv_obj_t *root = np_scene(parent);

    /* Conteudo de fundo, so para dar contexto ao mockup. */
    np_label(root, "14:30", NP_FONT_HERO, np_c_text_disabled(),
             NP_COL_X(0), NP_ROW_Y(0) + NP_SP_16, 300, LV_TEXT_ALIGN_LEFT);
    np_label(root, "Quinta-feira, 24 de julho", NP_FONT_LG, np_c_text_disabled(),
             NP_COL_X(0), NP_ROW_Y(2), NP_W_HALF, LV_TEXT_ALIGN_LEFT);

    /* Scrim: retangulo chapado, opacidade fixa. */
    np_fill(root, 0, 0, NP_SCREEN_W, NP_SCREEN_H, lv_color_hex(0x000000), LV_OPA_60, 0);

    /* Painel ancorado a direita, largura fixa. */
    const int32_t panel_w = 420;
    const int32_t panel_x = NP_SCREEN_W - panel_w;
    lv_obj_t *panel = np_fill(root, panel_x, 0, panel_w, NP_SCREEN_H,
                              np_c_surface(), LV_OPA_COVER, 0);
    np_vline(root, panel_x, 0, NP_SCREEN_H);

    const int32_t pad = NP_SP_32;
    const int32_t inner_w = panel_w - 2 * pad;

    np_label(panel, "Wi-Fi", NP_FONT_TITLE, np_c_text(),
             pad, pad, inner_w - 60, LV_TEXT_ALIGN_LEFT);
    np_label(panel, "X", NP_FONT_LG, np_c_text_3(),
             panel_w - pad - 40, pad + NP_SP_8, 40, LV_TEXT_ALIGN_RIGHT);

    np_label(panel, "Conectado a NovaNet", NP_FONT_SM, np_c_positive(),
             pad, pad + NP_SP_48, inner_w, LV_TEXT_ALIGN_LEFT);

    np_hline(panel, pad, pad + 96, inner_w);

    np_metric(panel, pad, pad + 120, inner_w, "Endereco IP", "192.168.1.16", NP_FONT_MD);
    np_metric(panel, pad, pad + 200, inner_w, "Sinal", "-42 dBm", NP_FONT_MD);
    np_metric(panel, pad, pad + 280, inner_w, "Transporte", "ESP-Hosted \xC2\xB7 SDIO", NP_FONT_MD);

    np_hline(panel, pad, pad + 372, inner_w);

    np_button(panel, pad, NP_SCREEN_H - pad - 52, inner_w, 52, "Trocar de rede", false);

    return root;
}
