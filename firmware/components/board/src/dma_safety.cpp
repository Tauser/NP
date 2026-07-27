// Matemática pura de alinhamento de DMA (ADR-010). SEM includes de IDF para
// permanecer host-testável (docs/TESTING.md). O log fica em dma_safe_assert.cpp.
#include "board/dma_safety.hpp"

namespace nova {
namespace board {

DmaSafety dma_check(uintptr_t base, size_t size, size_t cache_line) {
    DmaSafety r;
    if (cache_line == 0) {
        return r;  // linha desconhecida: cache_line_known_ fica false => inseguro
    }
    r.cache_line_known_ = true;
    r.base_rem_ = static_cast<size_t>(base % cache_line);
    r.size_rem_ = size % cache_line;
    r.base_aligned_ = (r.base_rem_ == 0);
    r.size_aligned_ = (r.size_rem_ == 0);
    return r;
}

}  // namespace board
}  // namespace nova
