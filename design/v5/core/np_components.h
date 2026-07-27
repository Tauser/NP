/*
 * NovaPanel v5 -- componentes (camada 3).
 *
 * Construtores que devolvem um POD so com os handles mutaveis. Um
 * componente NAO conhece estado de aplicacao, tela nem servico: recebe
 * tokens e geometria, devolve handles.
 *
 * Este e o unico arquivo (com np_styles.c) autorizado a chamar
 * lv_obj_set_style_* / lv_obj_create / lv_label_create. Arquivo de tela
 * nunca chama -- R10 do UI-LAYOUT-SYSTEM.md, verificavel por grep.
 *
 * R1: geometria decidida aqui, uma vez, no create. As funcoes *_set()
 *     so trocam conteudo dentro da caixa ja plantada.
 * R7: toda escrita compara antes de escrever (LVGL invalida o retangulo
 *     mesmo quando o valor e identico).
 */

#ifndef NP_COMPONENTS_H
#define NP_COMPONENTS_H

#include "lvgl.h"
#include "np_tokens.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---------------- escrita guardada (R7) ---------------- */

void np_set_text(lv_obj_t *label, const char *text);
void np_set_text_color(lv_obj_t *obj, lv_color_t color);
void np_set_bg_color(lv_obj_t *obj, lv_color_t color);

/* ---------------- primitivas ---------------- */

/* Raiz da cena: ocupa o parent inteiro, estilo de fundo aplicado. */
lv_obj_t *np_scene(lv_obj_t *parent);

/* Caixa sem estilo, para agrupar/posicionar (grupos de passo do Fluxo). */
lv_obj_t *np_group(lv_obj_t *parent, int32_t x, int32_t y, int32_t w, int32_t h);

lv_obj_t *np_surface(lv_obj_t *parent, int32_t x, int32_t y, int32_t w, int32_t h);
lv_obj_t *np_raised(lv_obj_t *parent, int32_t x, int32_t y, int32_t w, int32_t h);

/* Retangulo chapado de cor arbitraria -- e assim que "glow"/aura e feito
 * (R5: nunca lv_style_set_shadow_*). */
lv_obj_t *np_fill(lv_obj_t *parent, int32_t x, int32_t y, int32_t w, int32_t h,
                  lv_color_t color, lv_opa_t opa, int32_t radius);

/* Label com largura explicita e LV_LABEL_LONG_MODE_CLIP (R1/R2: nunca
 * LV_SIZE_CONTENT em ancestral de conteudo dinamico). */
lv_obj_t *np_label(lv_obj_t *parent, const char *text, const lv_font_t *font,
                   lv_color_t color, int32_t x, int32_t y, int32_t w,
                   lv_text_align_t align);

lv_obj_t *np_hline(lv_obj_t *parent, int32_t x, int32_t y, int32_t w);
lv_obj_t *np_vline(lv_obj_t *parent, int32_t x, int32_t y, int32_t h);
lv_obj_t *np_dot(lv_obj_t *parent, int32_t x, int32_t y, int32_t size, lv_color_t color);

/* ---------------- compostos ---------------- */

/* Rotulo pequeno + valor grande, alinhados a esquerda. */
typedef struct {
    lv_obj_t *caption;
    lv_obj_t *value;
} np_metric_t;

np_metric_t np_metric(lv_obj_t *parent, int32_t x, int32_t y, int32_t w,
                      const char *caption, const char *value, const lv_font_t *value_font);

/* Selo de estado do dado, no ponto de uso (nunca uma tela de erro global). */
typedef struct {
    lv_obj_t *dot;
    lv_obj_t *label;
} np_pill_t;

np_pill_t np_status_pill(lv_obj_t *parent, int32_t x, int32_t y, np_data_state_t state);
void np_status_pill_set(np_pill_t *pill, np_data_state_t state);

/* Relogio HH:MM, UM LABEL POR DIGITO (R8).
 *
 * Um tique tipico troca 1 digito (unidade de minuto), as vezes 2. Com um
 * label unico, LVGL invalida a caixa inteira e busca todos os glifos em
 * flash; cada miss de glyph bloqueia o acesso a PSRAM e rouba banda do
 * refresh do DSI (UI-LAYOUT-SYSTEM.md SS1.2). Por digito, o fetch por
 * minuto cai de ~5 glifos para ~1,2 em media. */
typedef struct {
    lv_obj_t *slot[5];   /* H H : M M */
} np_clock_t;

np_clock_t np_clock(lv_obj_t *parent, int32_t x, int32_t y, const lv_font_t *font,
                    lv_color_t color);
void np_clock_set(np_clock_t *clock, const char *hhmm);  /* exatamente 5 chars */

/* Botao. A tela liga o callback no objeto devolvido; o componente nao
 * conhece fila de acao. */
lv_obj_t *np_button(lv_obj_t *parent, int32_t x, int32_t y, int32_t w, int32_t h,
                    const char *text, bool primary);

/* Linha de lista reciclada (arquetipo Lista).
 *
 * O viewport tem N linhas de altura fixa criadas no build; rolar ou
 * atualizar re-texta as linhas existentes. Proibido criar um objeto por
 * item -- e o jeito mais facil de fazer o consumo de RAM virar funcao do
 * volume de dados sem ninguem perceber. */
typedef struct {
    lv_obj_t *root;
    lv_obj_t *lead;    /* hora, icone ou marcador a esquerda */
    lv_obj_t *title;
    lv_obj_t *meta;
    lv_obj_t *accent;  /* barra vertical de 3px, cor por categoria */
} np_row_t;

np_row_t np_row(lv_obj_t *parent, int32_t x, int32_t y, int32_t w, int32_t h);
void np_row_set(np_row_t *row, const char *lead, const char *title, const char *meta,
                lv_color_t accent_color, bool visible);

/* Tile de acao (arquetipo Grade). */
typedef struct {
    lv_obj_t *root;
    lv_obj_t *icon;
    lv_obj_t *title;
    lv_obj_t *state;
} np_tile_t;

np_tile_t np_tile(lv_obj_t *parent, int32_t x, int32_t y, int32_t w, int32_t h,
                  const char *icon, const char *title, const char *state);
void np_tile_set_on(np_tile_t *tile, bool on, const char *state_text);

/* Sparkline: polilinha simples a partir de amostras inteiras.
 *
 * Indicador de DIRECAO, nao grafico historico -- candles/OHLC saíram do
 * MVP (ADR-0019). Nunca desenhar com dado sintetico: se nao ha amostra
 * real, a tela esconde o componente. */
typedef struct {
    lv_obj_t *line;
    lv_point_precise_t pts[24];
} np_spark_t;

np_spark_t np_spark(lv_obj_t *parent, int32_t x, int32_t y, int32_t w, int32_t h);
void np_spark_set(np_spark_t *spark, const int32_t *samples, uint8_t count,
                  int32_t w, int32_t h, lv_color_t color);

/* Barra de progresso segmentada (arquetipo Fluxo: estagios do Boot). */
typedef struct {
    lv_obj_t *seg[6];
    uint8_t count;
} np_segbar_t;

np_segbar_t np_segbar(lv_obj_t *parent, int32_t x, int32_t y, int32_t w, uint8_t count);
void np_segbar_set(np_segbar_t *bar, uint8_t done, lv_color_t color);

/* Indicador de posicao entre cenas. */
lv_obj_t *np_dots(lv_obj_t *parent, int32_t y, uint8_t count, uint8_t active);

#ifdef __cplusplus
}
#endif

#endif /* NP_COMPONENTS_H */
