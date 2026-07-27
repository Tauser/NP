// Diagnóstico de boot (docs/GLITCH-PROTOCOLO.md §2.1, ADR-010/ADR-019).
// Loga, em nível INFO: alvo, revisão do silício, PSRAM, heap interno livre;
// e, por draw buffer, endereço-base/tamanho, resto por linha de cache,
// assert_dma_safe e o veredito da hipótese 2.1. Alvo-only.
#pragma once

#include "board/i_board.hpp"

namespace nova {
namespace diag {

// Retorna false se algum buffer for comprovadamente DMA-unsafe (base/size não
// múltiplos da linha de cache). Em `dev` o chamador deve tratar isso como boot
// abortado (RESOURCE-BUDGET §2.6); nunca chama abort() aqui (ARCHITECTURE §8).
bool run_boot_diagnostic(board::IBoard& board);

}  // namespace diag
}  // namespace nova
