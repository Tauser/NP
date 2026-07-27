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

// TODOS os buffers que o periférico lê precisam ser verificados, não só o que o
// LVGL está desenhando no momento: a ADR-019 exige "buffers de DMA desalinhados
// detectados no boot" como contador de gate. Com a receita vigente
// (esp_lvgl_adapter, TRIPLE_PARTIAL) são 3 framebuffers lidos pelo DSI mais o
// buffer parcial de desenho — por isso 4 slots.
struct DrawBufferReport {
    static constexpr size_t kMaxBuffers = 4;
    DrawBufferInfo buffers_[kMaxBuffers];
    size_t count_ = 0;
};

class IBoard {
public:
    virtual ~IBoard() = default;

    // Falha de display NÃO chama abort() (docs/ARCHITECTURE.md §8): retorna
    // false e o chamador decide a política de retry/reboot.
    virtual bool init_display() = 0;

    // Sobe somente o enlace P4<->C6 em uma task própria. Não associa Wi-Fi,
    // não faz NTP e não significa que há Internet: essas etapas dependem de
    // credenciais provisionadas e serviços ainda ausentes.
    virtual bool start_network_transport_async() = 0;
    virtual bool network_transport_ready() const = 0;

    // Lock semântico do LVGL/display. `lock_shared_i2c` pode ser o MESMO lock
    // por baixo (touch e codec dividem o I2C, RESOURCE-BUDGET §6) — mas só a
    // WaveshareBoard sabe disso; o resto do sistema conhece o nome semântico.
    virtual bool lock_ui(uint32_t timeout_ms) = 0;
    virtual void unlock_ui() = 0;
    virtual bool lock_shared_i2c(uint32_t timeout_ms) = 0;
    virtual void unlock_shared_i2c() = 0;

    virtual void set_brightness(int pct) = 0;
    virtual uint64_t rtc_unix_time_s() = 0;

    // Lê de volta os buffers lidos por DMA (framebuffers do painel + buffer de
    // desenho), para o diagnóstico de alinhamento no boot (ADR-010/ADR-019).
    virtual DrawBufferReport describe_draw_buffers() = 0;

};

}  // namespace board
}  // namespace nova
