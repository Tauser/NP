// Adaptador ESP-IDF para o contrato puro IHttpClient. Ele não é provider: só
// transporte, usado exclusivamente pela task NetworkWorker.
#pragma once

#include "utils/http_client.hpp"

namespace nova {
namespace services {

class EspHttpClient final : public utils::IHttpClient {
public:
    utils::Result<utils::HttpResponse> get(const utils::HttpRequest& request,
                                           utils::BoundedHttpBody& body) override;
};

}  // namespace services
}  // namespace nova
