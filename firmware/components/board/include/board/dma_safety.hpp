// Segurança de DMA vs. linha de cache (ADR-010, RESOURCE-BUDGET §2.6).
//
// O P4 tem linha de cache L2 de 64 B; um buffer que serve de origem/destino
// de DMA em PSRAM precisa de endereço-base E tamanho múltiplos da linha, senão
// o writeback de cache pode não cobrir o que o DMA lê. `dma_check` é matemática
// pura (host-testável); `assert_dma_safe` acrescenta log e é usada no boot.
#pragma once

#include <cstddef>
#include <cstdint>

namespace nova {
namespace board {

struct DmaSafety {
    size_t base_rem_ = 0;  // base % cache_line
    size_t size_rem_ = 0;  // size % cache_line
    bool base_aligned_ = false;
    bool size_aligned_ = false;
    bool cache_line_known_ = false;

    bool safe() const { return cache_line_known_ && base_aligned_ && size_aligned_; }
};

// Função pura. `cache_line == 0` => linha desconhecida => nunca "safe".
DmaSafety dma_check(uintptr_t base, size_t size, size_t cache_line);

// Loga o resultado (INFO se seguro, ERROR se não) e retorna `safe()`. A
// DECISÃO de abortar fica no chamador: abort() em board é proibido
// (docs/ARCHITECTURE.md §8). Em `dev` o chamador aborta o boot; em `prod`
// conta na métrica (ADR-019).
bool assert_dma_safe(uintptr_t base, size_t size, size_t cache_line, const char* what);

}  // namespace board
}  // namespace nova
