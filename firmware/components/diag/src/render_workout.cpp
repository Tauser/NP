// Gerador de conteúdo de render (alvo). Ver diag/render_workout.hpp.
//
// O "carimbo" (§3.1) é um contador monotônico grande, legível a quadro-a-quadro
// no vídeo, para casar o frame corrompido com o log serial. No modo torture
// (§3.2) uma barra de largura total troca de cor a cada tick, forçando o flush
// de uma faixa a cada atualização.
#include "diag/render_workout.hpp"

#include <cstdio>

#include "lvgl.h"

namespace nova {
namespace diag {

namespace {
// Estado do workout. Tocado só na lvgl_task (o timer roda nela) — sem race.
struct Workout {
    lv_obj_t* stamp_ = nullptr;
    lv_obj_t* band_ = nullptr;
    uint32_t frame_ = 0;
    bool aggressive_ = false;
    bool built_ = false;
};
Workout g_wk;

// Fonte do carimbo: grande no modo clareza (se a fonte estiver compilada),
// pequena no torture normal.
// NOVA_SMALL_STAMP: força fonte pequena no carimbo. Testa a hipótese de que o
// artefato é MISS DE CACHE DE GLIFO lendo flash (RESOURCE-BUDGET §1.2) e não
// tearing: glifo grande = mais bytes de flash por dígito novo = MSPI bloqueado
// = underrun do DSI. Mesma causa-raiz do defeito nº 1 (ADR-024).
#if defined(NOVA_CLARITY) && LV_FONT_MONTSERRAT_48 && !defined(NOVA_SMALL_STAMP)
const lv_font_t* stamp_font() { return &lv_font_montserrat_48; }
#else
const lv_font_t* stamp_font() { return &lv_font_montserrat_16; }
#endif

// No modo clareza, um rótulo estático diz o que é MEU. Assim, qualquer coisa
// fora do topo/canto que pisque ou vire ruído é inequivocamente o glitch.
void add_clarity_tag(lv_obj_t* scr) {
#ifdef NOVA_CLARITY
    lv_obj_t* tag = lv_label_create(scr);
    lv_obj_set_style_text_font(tag, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(tag, lv_color_white(), 0);
    lv_label_set_text(tag, "NOVA: topo e canto sao MEUS -- resto = defeito");
    lv_obj_align(tag, LV_ALIGN_CENTER, 0, 0);
#else
    (void)scr;
#endif
}

void build() {
    lv_obj_t* scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);

    // Barra de faixa: geometria FINAL definida aqui (só a cor muda no update).
    g_wk.band_ = lv_obj_create(scr);
    lv_obj_set_size(g_wk.band_, LV_HOR_RES, 80);
    lv_obj_align(g_wk.band_, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_border_width(g_wk.band_, 0, 0);

    // Carimbo: contador no topo (grande no modo clareza).
    g_wk.stamp_ = lv_label_create(scr);
    lv_obj_set_style_text_font(g_wk.stamp_, stamp_font(), 0);
    lv_obj_set_style_text_color(g_wk.stamp_, lv_color_white(), 0);
    // Geometria FINAL no build() (UI-PATTERN §10): caixa de LARGURA FIXA. Boa
    // prática, mas NÃO era a causa das "listras" (testado: não mudou). A causa
    // está sob investigação — ver NOVA_STATIC_STAMP e GLITCH-PROTOCOLO §2.3.
    lv_obj_set_width(g_wk.stamp_, 480);
    lv_label_set_long_mode(g_wk.stamp_, LV_LABEL_LONG_CLIP);
    lv_obj_align(g_wk.stamp_, LV_ALIGN_TOP_LEFT, 8, 8);
    lv_label_set_text(g_wk.stamp_, "F000000");  // conteúdo inicial (para o modo estático)

    add_clarity_tag(scr);
    g_wk.built_ = true;
}

void tick(lv_timer_t*) {
    if (!g_wk.built_) {
        build();
    }
    ++g_wk.frame_;
#ifndef NOVA_STATIC_STAMP
    // Carimbo: F<contador> zero-padded => COMPRIMENTO FIXO sempre (§3.1).
    //
    // Montado com snprintf da libc e aplicado com lv_label_set_text — a MESMA
    // API do caso estático. Antes usava lv_label_set_text_fmt, o que fazia a
    // bisseção "estático vs atualizando" trocar duas variáveis de uma vez
    // (atualizar E a API de texto). Agora a única diferença é atualizar ou não.
    char buf[16];
    std::snprintf(buf, sizeof(buf), "F%06lu", static_cast<unsigned long>(g_wk.frame_));
    lv_label_set_text(g_wk.stamp_, buf);
#endif
    if (g_wk.aggressive_) {
        // Alterna a cor da faixa -> invalida e faz flush de ~80 linhas por tick.
        const bool odd = (g_wk.frame_ & 1u) != 0u;
        lv_obj_set_style_bg_color(g_wk.band_, odd ? lv_color_hex(0xE00000) : lv_color_hex(0x0000E0), 0);
    }
}
}  // namespace

void start_render_workout(bool aggressive, uint32_t period_ms) {
    g_wk.aggressive_ = aggressive;
    lv_timer_create(tick, period_ms, nullptr);
}

}  // namespace diag
}  // namespace nova
