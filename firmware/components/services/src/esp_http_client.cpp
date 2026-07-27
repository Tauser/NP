#include "services/esp_http_client.hpp"

#include <cstring>

#include "esp_crt_bundle.h"
#include "esp_err.h"
#include "esp_http_client.h"

namespace nova {
namespace services {

namespace {
struct ReceiveContext {
    utils::BoundedHttpBody* body_ = nullptr;
};

bool is_https_url(const char* url) {
    return url != nullptr && std::strncmp(url, "https://", 8) == 0;
}

utils::Status translate_error(esp_err_t error) {
    if (error == ESP_ERR_TIMEOUT) {
        return utils::Status::kTimeout;
    }
    if (error == ESP_ERR_NO_MEM) {
        return utils::Status::kNoMemory;
    }
    return utils::Status::kNetworkDown;
}

esp_err_t on_http_event(esp_http_client_event_t* event) {
    if (event == nullptr || event->event_id != HTTP_EVENT_ON_DATA) {
        return ESP_OK;
    }
    auto* context = static_cast<ReceiveContext*>(event->user_data);
    if (context == nullptr || context->body_ == nullptr || event->data_len < 0) {
        return ESP_FAIL;
    }
    return context->body_->append(static_cast<const uint8_t*>(event->data),
                                  static_cast<size_t>(event->data_len)) == utils::Status::kOk
               ? ESP_OK
               : ESP_FAIL;
}
}  // namespace

utils::Result<utils::HttpResponse> EspHttpClient::get(const utils::HttpRequest& request,
                                                       utils::BoundedHttpBody& body) {
    if (!is_https_url(request.url) || body.reset() != utils::Status::kOk) {
        return utils::Result<utils::HttpResponse>::fail(utils::Status::kInvalidArg);
    }
    ReceiveContext context{&body};
    esp_http_client_config_t config = {};
    config.url = request.url;
    config.timeout_ms = static_cast<int>(request.timeout_ms);
    config.event_handler = on_http_event;
    config.user_data = &context;
    config.crt_bundle_attach = esp_crt_bundle_attach;
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == nullptr) {
        return utils::Result<utils::HttpResponse>::fail(utils::Status::kNoMemory);
    }
    const esp_err_t error = esp_http_client_perform(client);
    const int status_code = esp_http_client_get_status_code(client);
    esp_http_client_cleanup(client);
    if (body.status() != utils::Status::kOk) {
        return utils::Result<utils::HttpResponse>::fail(body.status());
    }
    if (error != ESP_OK) {
        return utils::Result<utils::HttpResponse>::fail(translate_error(error));
    }
    if (status_code < 200 || status_code >= 300) {
        return utils::Result<utils::HttpResponse>::fail(utils::Status::kHttpError);
    }
    return body.finish(static_cast<uint16_t>(status_code));
}

}  // namespace services
}  // namespace nova
