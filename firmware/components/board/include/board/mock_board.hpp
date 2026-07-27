// MockBoard: implementação de IBoard sem placa, para host (docs/ARCHITECTURE.md
// §4/§14). Header-only e puro — exercitada pelos testes nativos.
#pragma once

#include "board/i_board.hpp"

namespace nova {
namespace board {

class MockBoard : public IBoard {
public:
    // Draw buffers sintéticos configuráveis, para testar a lógica de
    // alinhamento contra endereços conhecidos sem hardware.
    void set_draw_buffer(uintptr_t base, size_t size) {
        report_.buffers_[0].base_ = base;
        report_.buffers_[0].size_ = size;
        report_.count_ = 1;
    }

    bool init_display() override {
        display_ready_ = true;
        return true;
    }

    bool lock_ui(uint32_t) override {
        ++ui_locks_;
        return true;
    }
    void unlock_ui() override {}
    bool lock_shared_i2c(uint32_t) override {
        ++i2c_locks_;
        return true;
    }
    void unlock_shared_i2c() override {}

    void set_brightness(int pct) override { brightness_pct_ = pct; }
    uint64_t rtc_unix_time_s() override { return rtc_s_; }

    DrawBufferReport describe_draw_buffers() override { return report_; }

    // Observáveis para os testes.
    bool display_ready_ = false;
    int brightness_pct_ = -1;
    uint64_t rtc_s_ = 0;
    unsigned ui_locks_ = 0;
    unsigned i2c_locks_ = 0;

private:
    DrawBufferReport report_;
};

}  // namespace board
}  // namespace nova
