// Fonte de UTC para o ClockService futuro (ADR-007 e ADR-015).
//
// O provider só obtém/traduz o dado. Prioridade, retry, cache e mutação do
// estado são responsabilidades do service + RequestOrchestrator, nunca daqui.
#pragma once

#include "models/time.hpp"
#include "utils/result.hpp"

namespace nova {
namespace providers {

class ITimeProvider {
public:
    virtual ~ITimeProvider() = default;
    virtual utils::Result<models::UtcTime> fetch_utc_time() = 0;
};

}  // namespace providers
}  // namespace nova
