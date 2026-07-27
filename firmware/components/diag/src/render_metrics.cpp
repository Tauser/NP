// Lógica pura das métricas de render (host-testável). Sem includes de IDF.
#include "diag/render_metrics.hpp"

#include <algorithm>

namespace nova {
namespace diag {

void RenderMetrics::on_update(int64_t dur_us) {
    ++updates_;
    update_last_us_ = dur_us;
    if (dur_us > update_max_us_) {
        update_max_us_ = dur_us;
    }
    ring_[ring_pos_] = dur_us;
    ring_pos_ = (ring_pos_ + 1) % kRing;
    if (ring_len_ < kRing) {
        ++ring_len_;
    }
    last_flushes_ = flushes_in_update_;
    if (flushes_in_update_ > max_flushes_) {
        max_flushes_ = flushes_in_update_;
    }
    flushes_in_update_ = 0;
}

void RenderMetrics::on_flush_wait(int64_t dur_us) {
    wait_last_us_ = dur_us;
    if (dur_us > wait_max_us_) {
        wait_max_us_ = dur_us;
    }
}

int64_t RenderMetrics::percentile_us(int pct) const {
    if (ring_len_ == 0) {
        return 0;
    }
    int64_t sorted[kRing];
    std::copy(ring_, ring_ + ring_len_, sorted);
    std::sort(sorted, sorted + ring_len_);
    size_t idx = static_cast<size_t>((static_cast<int64_t>(pct) * (static_cast<int64_t>(ring_len_) - 1)) / 100);
    return sorted[idx];
}

}  // namespace diag
}  // namespace nova
