// WaveshareBoard: IBoard real sobre o BSP esp32_p4_wifi6_touch_lcd_7b e o
// esp_lvgl_port. É o ÚNICO lugar que conhece a coincidência física "lock do
// display == mutex do I2C compartilhado" (RESOURCE-BUDGET §6).
//
// Alvo-only (puxa LVGL/IDF). Não incluir no host_check.
#pragma once

#include "board/i_board.hpp"

struct _lv_display_t;  // fwd: evita puxar lvgl.h para quem só usa a interface

namespace nova {
namespace board {

class WaveshareBoard : public IBoard {
public:
    bool init_display() override;
    bool lock_ui(uint32_t timeout_ms) override;
    void unlock_ui() override;
    bool lock_shared_i2c(uint32_t timeout_ms) override;
    void unlock_shared_i2c() override;
    void set_brightness(int pct) override;
    uint64_t rtc_unix_time_s() override;
    DrawBufferReport describe_draw_buffers() override;

private:
    _lv_display_t* disp_ = nullptr;  // dono: esta board; lido só após init_display
};

}  // namespace board
}  // namespace nova
