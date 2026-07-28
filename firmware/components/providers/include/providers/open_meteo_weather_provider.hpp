// Adapter Open-Meteo para Brasilia/DF. O endpoint e os campos sao fixos para
// manter resposta pequena e parser auditavel; personalizacao vira setup futuro.
#pragma once

#include <cstddef>
#include <cstdint>

#include "providers/i_weather_provider.hpp"

namespace nova {
namespace providers {

class OpenMeteoWeatherProvider final : public IWeatherProvider {
public:
    utils::Result<models::WeatherState> fetch_current(utils::IHttpClient& client,
                                                        utils::BoundedHttpBody& body) override;

    // Puro e exposto para fixtures host; nunca aceita JSON truncado como dado.
    static utils::Result<models::WeatherState> parse_current(const uint8_t* body, size_t size);
};

}  // namespace providers
}  // namespace nova
