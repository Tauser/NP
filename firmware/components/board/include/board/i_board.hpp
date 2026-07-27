// HAL da NovaPanel (docs/ARCHITECTURE.md §4, ADR-005).
//
// Interface PURA: nenhum tipo de IDF ou LVGL vaza para cá, para que MockBoard
// e a lógica de diagnóstico compilem no host (docs/TESTING.md). Todo o
// hardware fica atrás desta interface; `main/` é só wiring.
#pragma once

#include <cstddef>
#include <cstdint>

namespace nova {
namespace board {

// Descrição pura de um draw buffer do LVGL, para o diagnóstico de alinhamento
// de DMA no boot (ADR-010, docs/GLITCH-PROTOCOLO.md §2.1). `base_ == 0`
// significa "buffer ausente".
struct DrawBufferInfo {
    uintptr_t base_ = 0;  // endereço-base
    size_t size_ = 0;     // bytes
};

// Com a receita da plataforma (double_buffer=false, RESOURCE-BUDGET §2.5) há
// um único draw buffer. O segundo slot existe só para não mentir sobre a
// forma quando alguém habilitar double buffer no futuro.
struct DrawBufferReport {
    DrawBufferInfo buffers_[2];
    size_t count_ = 0;
};

class IBoard {
public:
    virtual ~IBoard() = default;

    // Falha de display NÃO chama abort() (docs/ARCHITECTURE.md §8): retorna
    // false e o chamador decide a política de retry/reboot.
    virtual bool init_display() = 0;

    // Lock semântico do LVGL/display. `lock_shared_i2c` pode ser o MESMO lock
    // por baixo (touch e codec dividem o I2C, RESOURCE-BUDGET §6) — mas só a
    // WaveshareBoard sabe disso; o resto do sistema conhece o nome semântico.
    virtual bool lock_ui(uint32_t timeout_ms) = 0;
    virtual void unlock_ui() = 0;
    virtual bool lock_shared_i2c(uint32_t timeout_ms) = 0;
    virtual void unlock_shared_i2c() = 0;

    virtual void set_brightness(int pct) = 0;
    virtual uint64_t rtc_unix_time_s() = 0;

    // Lê de volta os draw buffers que o esp_lvgl_port alocou, para o
    // diagnóstico de alinhamento (via API pública do LVGL na impl. real).
    virtual DrawBufferReport describe_draw_buffers() = 0;

    // NOTA DE ESCOPO (Onda 0): `start_network_transport_async()` e `audio()`
    // do §4 estão ADIADOS — não há rede nem áudio nesta onda (ADR-023) e
    // atribuir o glitch não precisa deles. Entram nas ondas A/posteriores.
};

}  // namespace board
}  // namespace nova
