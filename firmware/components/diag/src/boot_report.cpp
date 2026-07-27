// Lógica pura do veredito de alinhamento (host-testável). Sem includes de IDF.
#include "diag/boot_report.hpp"

namespace nova {
namespace diag {

AlignVerdict classify_alignment(size_t draw_buf_align, uintptr_t base, size_t cache_line) {
    if (cache_line == 0) {
        return AlignVerdict::kUnknownCacheLine;
    }
    if (draw_buf_align >= cache_line) {
        // Alinhado porque o Kconfig obriga; este build não testa 2.1.
        return AlignVerdict::kAlignedByConfig;
    }
    if (base % cache_line == 0) {
        return AlignVerdict::kAlignedByLuck;
    }
    return AlignVerdict::kMisaligned;
}

bool flash_supports_suspend(uint32_t chip_id) {
    // IDs que o driver GD do ESP-IDF marca com SPI_FLASH_CHIP_CAP_SUSPEND.
    switch (chip_id) {
        case 0xC84016u:
        case 0xC84017u:
        case 0xC84018u:
        case 0xC84319u:
            return true;
        default:
            return false;
    }
}

const char* verdict_text(AlignVerdict v) {
    switch (v) {
        case AlignVerdict::kUnknownCacheLine:
            return "linha de cache desconhecida — nada a concluir sobre 2.1";
        case AlignVerdict::kAlignedByConfig:
            return "alinhado POR CONFIG (align>=linha): este build NAO testa a "
                   "hipotese 2.1 — para testar, rebuild com CONFIG_LV_DRAW_BUF_ALIGN=4";
        case AlignVerdict::kAlignedByLuck:
            return "alinhado por SORTE do alocador (align<linha) — hipotese 2.1 CAI";
        case AlignVerdict::kMisaligned:
            return "DESALINHADO (base%linha!=0) — hipotese 2.1 VIVA";
    }
    return "?";
}

}  // namespace diag
}  // namespace nova
