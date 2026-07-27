#include "np_styles.h"
#include "np_tokens.h"

static bool s_ready = false;

static lv_style_t s_screen;
static lv_style_t s_surface;
static lv_style_t s_raised;
static lv_style_t s_hairline;
static lv_style_t s_label;
static lv_style_t s_tile;
static lv_style_t s_tile_on;
static lv_style_t s_row;

void np_styles_init(void)
{
    if (s_ready) {
        return;
    }
    s_ready = true;

    lv_style_init(&s_screen);
    lv_style_set_bg_color(&s_screen, np_c_bg());
    lv_style_set_bg_opa(&s_screen, LV_OPA_COVER);
    lv_style_set_border_width(&s_screen, 0);
    lv_style_set_radius(&s_screen, 0);
    lv_style_set_pad_all(&s_screen, 0);

    /* R5: shadow_width e sempre 0. Com CONFIG_LV_DRAW_SW_SHADOW_CACHE_SIZE=0
     * toda sombra e recalculada a cada repintura E infla o retangulo sujo
     * pelo spread -- dois custos, nenhum ganho que um retangulo chapado nao
     * entregue. */
    lv_style_init(&s_surface);
    lv_style_set_bg_color(&s_surface, np_c_surface());
    lv_style_set_bg_opa(&s_surface, LV_OPA_COVER);
    lv_style_set_border_color(&s_surface, np_c_hairline());
    lv_style_set_border_width(&s_surface, 1);
    lv_style_set_radius(&s_surface, NP_RADIUS_SURFACE);
    lv_style_set_shadow_width(&s_surface, 0);
    lv_style_set_pad_all(&s_surface, NP_SP_24);

    lv_style_init(&s_raised);
    lv_style_set_bg_color(&s_raised, np_c_surface_raised());
    lv_style_set_bg_opa(&s_raised, LV_OPA_COVER);
    lv_style_set_border_color(&s_raised, np_c_hairline());
    lv_style_set_border_width(&s_raised, 1);
    lv_style_set_radius(&s_raised, NP_RADIUS_SURFACE);
    lv_style_set_shadow_width(&s_raised, 0);
    lv_style_set_pad_all(&s_raised, NP_SP_24);

    lv_style_init(&s_hairline);
    lv_style_set_bg_color(&s_hairline, np_c_hairline());
    lv_style_set_bg_opa(&s_hairline, LV_OPA_COVER);
    lv_style_set_border_width(&s_hairline, 0);
    lv_style_set_radius(&s_hairline, 0);
    lv_style_set_pad_all(&s_hairline, 0);

    lv_style_init(&s_label);
    lv_style_set_bg_opa(&s_label, LV_OPA_TRANSP);
    lv_style_set_border_width(&s_label, 0);
    lv_style_set_pad_all(&s_label, 0);
    lv_style_set_text_color(&s_label, np_c_text());
    lv_style_set_text_font(&s_label, NP_FONT_MD);

    lv_style_init(&s_tile);
    lv_style_set_bg_color(&s_tile, np_c_surface());
    lv_style_set_bg_opa(&s_tile, LV_OPA_COVER);
    lv_style_set_border_color(&s_tile, np_c_hairline());
    lv_style_set_border_width(&s_tile, 1);
    lv_style_set_radius(&s_tile, NP_RADIUS_CONTROL);
    lv_style_set_shadow_width(&s_tile, 0);
    lv_style_set_pad_all(&s_tile, NP_SP_16);

    lv_style_init(&s_tile_on);
    lv_style_set_bg_color(&s_tile_on, np_c_accent_bg());
    lv_style_set_bg_opa(&s_tile_on, LV_OPA_COVER);
    lv_style_set_border_color(&s_tile_on, np_c_accent());
    lv_style_set_border_width(&s_tile_on, 1);
    lv_style_set_radius(&s_tile_on, NP_RADIUS_CONTROL);
    lv_style_set_shadow_width(&s_tile_on, 0);
    lv_style_set_pad_all(&s_tile_on, NP_SP_16);

    lv_style_init(&s_row);
    lv_style_set_bg_color(&s_row, np_c_surface());
    lv_style_set_bg_opa(&s_row, LV_OPA_COVER);
    lv_style_set_border_color(&s_row, np_c_hairline());
    lv_style_set_border_width(&s_row, 1);
    lv_style_set_radius(&s_row, NP_RADIUS_CONTROL);
    lv_style_set_shadow_width(&s_row, 0);
    lv_style_set_pad_all(&s_row, 0);
}

#define NP_STYLE_ACCESSOR(fn, obj)            \
    const lv_style_t *fn(void)                \
    {                                         \
        np_styles_init();                     \
        return &(obj);                        \
    }

NP_STYLE_ACCESSOR(np_st_screen, s_screen)
NP_STYLE_ACCESSOR(np_st_surface, s_surface)
NP_STYLE_ACCESSOR(np_st_raised, s_raised)
NP_STYLE_ACCESSOR(np_st_hairline, s_hairline)
NP_STYLE_ACCESSOR(np_st_label, s_label)
NP_STYLE_ACCESSOR(np_st_tile, s_tile)
NP_STYLE_ACCESSOR(np_st_tile_on, s_tile_on)
NP_STYLE_ACCESSOR(np_st_row, s_row)
