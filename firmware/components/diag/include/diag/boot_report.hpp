// Veredito de alinhamento do draw buffer (docs/GLITCH-PROTOCOLO.md §2.1).
// Unidade PURA e host-testável — é a lógica que impede o diagnóstico de MENTIR
// sobre a hipótese 2.1.
//
// Ponto crítico: o sdkconfig.defaults JÁ fixa CONFIG_LV_DRAW_BUF_ALIGN=64
// (ADR-010). Logo, no build default o endereço é alinhado POR CONSTRUÇÃO e não
// diz nada sobre a hipótese 2.1. A falsificação "alinhado por sorte" só tem
// sentido num build com align=4. Este classificador torna isso explícito.
#pragma once

#include <cstddef>
#include <cstdint>

namespace nova {
namespace diag {

enum class AlignVerdict {
    kUnknownCacheLine,  // linha de cache desconhecida: nada a concluir
    kAlignedByConfig,   // draw_buf_align >= linha: alinhado por construção
    kAlignedByLuck,     // align < linha, mas base % linha == 0 -> 2.1 CAI
    kMisaligned,        // base % linha != 0 -> hipótese 2.1 VIVA
};

// draw_buf_align = CONFIG_LV_DRAW_BUF_ALIGN em efeito no binário.
// cache_line     = CONFIG_CACHE_L2_CACHE_LINE_SIZE (0 se desconhecida).
AlignVerdict classify_alignment(size_t draw_buf_align, uintptr_t base, size_t cache_line);

// Frase curta para o log. Deixa claro o que este build PODE ou NÃO PODE concluir.
const char* verdict_text(AlignVerdict v);

// Defeito nº 1 (ADR-024): erase de flash bloqueia o MSPI e o DSI sofre underrun.
// A única mitigação de hardware é o auto-suspend do erase, que exige um chip de
// flash específico. Lista do driver GD do ESP-IDF (spi_flash_chip_gd.c).
// Puro: recebe o chip_id lido em runtime e diz se o suspend é possível.
bool flash_supports_suspend(uint32_t chip_id);

}  // namespace diag
}  // namespace nova
