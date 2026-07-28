#include "services/usb_wifi_provisioner.hpp"

#include <cstring>
#include <unistd.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "services/usb_wifi_provisioning_protocol.hpp"

namespace nova {
namespace services {
namespace {
constexpr const char* kTag = "usb.provision";
constexpr uint32_t kTaskStackBytes = 4096;
constexpr UBaseType_t kTaskPriority = 2;
constexpr size_t kMaxFrameBytes = 192;
}  // namespace

bool UsbWifiProvisioner::start() {
    if (started_) return true;
    if (xTaskCreate(task_entry, "usb_provision", kTaskStackBytes, this, kTaskPriority, nullptr) != pdPASS) {
        ESP_LOGE(kTag, "nao criou tarefa de provisionamento USB");
        return false;
    }
    started_ = true;
    ESP_LOGI(kTag, "pronto para frame NPW1 via USB fisico");
    return true;
}

void UsbWifiProvisioner::task_entry(void* context) {
    static_cast<UsbWifiProvisioner*>(context)->run();
}

void UsbWifiProvisioner::run() {
    char frame[kMaxFrameBytes] = {};
    size_t length = 0;
    for (;;) {
        char byte = 0;
        const ssize_t read_count = read(STDIN_FILENO, &byte, 1);
        if (read_count != 1) {
            vTaskDelay(pdMS_TO_TICKS(20));
            continue;
        }
        if (byte == '\r') continue;
        if (byte != '\n') {
            if (length + 1 < sizeof(frame)) frame[length++] = byte;
            else length = 0;  // descarta frame gigante, sem guardar segredo parcial
            continue;
        }

        board::WifiCredentials credentials;
        const utils::Status parsed =
            parse_usb_wifi_provisioning_frame(frame, length, credentials);
        std::memset(frame, 0, sizeof(frame));
        length = 0;
        if (parsed != utils::Status::kOk || !mailbox_.submit(credentials)) {
            ESP_LOGW(kTag, "frame USB recusado");
        } else {
            ESP_LOGI(kTag, "credencial USB recebida; aguardando app_loop");
        }
        credentials = {};
    }
}

}  // namespace services
}  // namespace nova
