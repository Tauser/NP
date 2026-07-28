// Sincroniza UTC por SNTP depois de o Wi-Fi receber IP. I/O e entrega da hora
// pertencem a app_loop: nenhum callback do driver escreve estado.
#pragma once

#include <cstdint>

#include "board/i_board.hpp"
#include "core/service_manager.hpp"
#include "services/clock_service.hpp"

namespace nova {
namespace services {

class SntpService final : public core::IAppService {
public:
    SntpService(board::IBoard& board, ClockService& clock);

    utils::Status start() override;
    void tick(uint64_t now_ms) override;

private:
    board::IBoard& board_;
    ClockService& clock_;
    bool initialized_ = false;
    bool synchronized_ = false;
};

}  // namespace services
}  // namespace nova
