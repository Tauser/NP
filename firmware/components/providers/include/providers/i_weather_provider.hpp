// Contrato do domínio de clima. I/O e parse pertencem ao provider; agenda e
// estado pertencem ao WeatherService (ARCHITECTURE §2).
#pragma once

#include "models/app_state.hpp"
#include "utils/http_client.hpp"

namespace nova {
namespace providers {

class IWeatherProvider {
public:
    virtual ~IWeatherProvider() = default;
    virtual utils::Result<models::WeatherState> fetch_current(utils::IHttpClient& client,
                                                                utils::BoundedHttpBody& body) = 0;
};

}  // namespace providers
}  // namespace nova
