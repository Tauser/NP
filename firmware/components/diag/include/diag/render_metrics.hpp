// Instrumentação permanente de render (docs/GLITCH-PROTOCOLO.md §3.3).
// "Números, não adjetivos." Unidade PURA: recebe timestamps já medidos, para
// ser host-testável. Quem mede o tempo é diag/render_probe (alvo).
//
// PROPRIEDADE (ADR-008/ADR-011): todos os campos são tocados APENAS na
// lvgl_task — os eventos de render e o dump periódico rodam nela. Não há acesso
// cross-task a esta struct, por isso os campos não são atômicos.
#pragma once

#include <cstddef>
#include <cstdint>

namespace nova {
namespace diag {

class RenderMetrics {
public:
    void on_flush() {
        ++flushes_total_;
        ++flushes_in_update_;
    }

    // Chamado ao fim de cada render (LV_EVENT_RENDER_READY) com a duração já
    // calculada. Fecha a contagem de flushes daquele update.
    void on_update(int64_t dur_us);

    void on_flush_wait(int64_t dur_us);

    // p50/p95 sobre a janela recente (ring de kRing amostras).
    int64_t percentile_us(int pct) const;

    uint64_t updates() const { return updates_; }
    uint64_t flushes_total() const { return flushes_total_; }
    uint32_t last_flushes() const { return last_flushes_; }
    uint32_t max_flushes() const { return max_flushes_; }
    int64_t update_last_us() const { return update_last_us_; }
    int64_t update_max_us() const { return update_max_us_; }
    int64_t wait_last_us() const { return wait_last_us_; }
    int64_t wait_max_us() const { return wait_max_us_; }

private:
    static constexpr size_t kRing = 64;

    uint64_t updates_ = 0;
    uint64_t flushes_total_ = 0;
    uint32_t flushes_in_update_ = 0;
    uint32_t last_flushes_ = 0;
    uint32_t max_flushes_ = 0;
    int64_t update_last_us_ = 0;
    int64_t update_max_us_ = 0;
    int64_t wait_last_us_ = 0;
    int64_t wait_max_us_ = 0;
    int64_t ring_[kRing] = {};
    size_t ring_len_ = 0;
    size_t ring_pos_ = 0;
};

}  // namespace diag
}  // namespace nova
