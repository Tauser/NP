// Contrato de tempo UTC vindo de uma fonte externa (RTC, NTP ou mock).
//
// Não é AppState: o service converte UTC para ClockState local e só então
// publica pelo StateStore. Manter esta fronteira evita expor o protocolo de
// uma fonte de tempo à UI.
#pragma once

#include <cstdint>

namespace nova {
namespace models {

struct UtcTime {
    uint64_t unix_time_s = 0;
};

}  // namespace models
}  // namespace nova
