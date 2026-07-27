// Invólucro alvo-only da task do NetworkWorker. Isola FreeRTOS e a alocação
// explícita em SRAM interna da lógica host-testável em network_worker.cpp.
#pragma once

#include "services/network_worker.hpp"

namespace nova {
namespace services {

class NetworkWorkerTask {
public:
    explicit NetworkWorkerTask(NetworkWorker& worker);
    bool start();

private:
    static void task_entry(void* context);
    void run();

    NetworkWorker& worker_;
    uint8_t* body_storage_ = nullptr;
    bool started_ = false;
};

}  // namespace services
}  // namespace nova
