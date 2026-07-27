// Testes nativos da lógica pura da Onda 0 (docs/TESTING.md). Rodam sem placa.
// Cobrem: alinhamento de DMA (ADR-010), veredito da hipótese 2.1 (GLITCH §2.1),
// métricas de render (GLITCH §3.3) e MockBoard (ARCHITECTURE §4).
#include <cstdio>

#include "board/dma_safety.hpp"
#include "board/mock_board.hpp"
#include "diag/boot_report.hpp"
#include "diag/render_metrics.hpp"

namespace {
int g_fail = 0;

void check(bool cond, const char* what) {
    if (!cond) {
        std::printf("  FAIL: %s\n", what);
        ++g_fail;
    }
}

void test_dma_check() {
    using nova::board::dma_check;
    // 1024×60×2B = 122880 (múltiplo de 64); base alinhada -> safe.
    auto a = dma_check(0x40000000u, 122880u, 64u);
    check(a.safe(), "dma_check base/size alinhados => safe");
    // base desalinhada por 4 (o cenário histórico com align=4).
    auto b = dma_check(0x40000004u, 122880u, 64u);
    check(!b.safe() && b.base_rem_ == 4u, "dma_check base%64==4 => unsafe");
    // tamanho desalinhado.
    auto c = dma_check(0x40000000u, 122880u + 1u, 64u);
    check(!c.safe() && c.size_rem_ == 1u, "dma_check size%64!=0 => unsafe");
    // linha desconhecida nunca é safe.
    auto d = dma_check(0x40000000u, 122880u, 0u);
    check(!d.safe() && !d.cache_line_known_, "dma_check linha=0 => unsafe/desconhecida");
}

void test_verdict() {
    using namespace nova::diag;
    // align=64 já fixado no sdkconfig: alinhado por construção, NÃO testa 2.1.
    check(classify_alignment(64, 0x40002000u, 64) == AlignVerdict::kAlignedByConfig,
          "verdict align>=linha => AlignedByConfig");
    // align=4 e base%64==0: hipótese 2.1 cai.
    check(classify_alignment(4, 0x40002000u, 64) == AlignVerdict::kAlignedByLuck,
          "verdict align<linha & base%64==0 => AlignedByLuck");
    // align=4 e base%64!=0: hipótese 2.1 viva.
    check(classify_alignment(4, 0x40002004u, 64) == AlignVerdict::kMisaligned,
          "verdict base%64!=0 => Misaligned");
    // linha desconhecida.
    check(classify_alignment(64, 0x40002000u, 0) == AlignVerdict::kUnknownCacheLine,
          "verdict linha=0 => UnknownCacheLine");
}

void test_flash_suspend() {
    using nova::diag::flash_supports_suspend;
    // IDs que o driver GD do ESP-IDF marca como suspend-capable.
    check(flash_supports_suspend(0xC84016u), "GD 0xC84016 suporta suspend");
    check(flash_supports_suspend(0xC84319u), "GD 0xC84319 suporta suspend");
    // 0xC84019 (32MB comum) NAO esta na lista -> sem mitigacao de hardware.
    check(!flash_supports_suspend(0xC84019u), "GD 0xC84019 NAO suporta suspend");
    check(!flash_supports_suspend(0u), "id 0 nao suporta suspend");
}

void test_metrics() {
    nova::diag::RenderMetrics m;
    m.on_flush();
    m.on_flush();
    m.on_flush();
    m.on_update(1000);
    check(m.updates() == 1, "metrics 1 update");
    check(m.flushes_total() == 3, "metrics 3 flushes totais");
    check(m.last_flushes() == 3 && m.max_flushes() == 3, "metrics 3 flushes no update");
    check(m.update_max_us() == 1000, "metrics dur max");
    m.on_update(10);
    m.on_update(30);
    m.on_update(20);  // ring agora: 1000,10,30,20
    check(m.percentile_us(0) == 10, "metrics p0 == min");
    check(m.percentile_us(100) == 1000, "metrics p100 == max");
    m.on_flush_wait(500);
    check(m.wait_last_us() == 500 && m.wait_max_us() == 500, "metrics espera flush");
}

void test_mock_board() {
    nova::board::MockBoard mb;
    check(mb.init_display() && mb.display_ready_, "mock init_display");
    mb.set_brightness(80);
    check(mb.brightness_pct_ == 80, "mock set_brightness");
    check(mb.lock_ui(0) && mb.ui_locks_ == 1, "mock lock_ui");
    check(mb.lock_shared_i2c(0) && mb.i2c_locks_ == 1, "mock lock_shared_i2c");
    mb.set_draw_buffer(0x40000000u, 122880u);
    auto rep = mb.describe_draw_buffers();
    check(rep.count_ == 1 && rep.buffers_[0].size_ == 122880u, "mock describe_draw_buffers");
}
}  // namespace

int main() {
    std::printf("native tests:\n");
    test_dma_check();
    test_verdict();
    test_flash_suspend();
    test_metrics();
    test_mock_board();
    if (g_fail == 0) {
        std::printf("  PASS (todos)\n");
        return 0;
    }
    std::printf("  %d checagens falharam\n", g_fail);
    return 1;
}
