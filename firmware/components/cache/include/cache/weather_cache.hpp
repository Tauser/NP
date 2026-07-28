// Cache de clima: contrato puro mais adaptador LittleFS. O service decide
// quando persistir; o cache apenas valida e conserva o blob versionado.
#pragma once

#include <cstddef>
#include <cstdint>

#include "models/app_state.hpp"
#include "utils/result.hpp"

namespace nova {
namespace cache {

struct WeatherCacheEntry {
    models::WeatherState weather_;
    uint64_t saved_utc_s_ = 0;
};

class IWeatherCache {
public:
    virtual ~IWeatherCache() = default;
    virtual utils::Result<WeatherCacheEntry> load() = 0;
    virtual utils::Status save(const WeatherCacheEntry& entry) = 0;
};

// Formato fixo em little-endian: header 12 B + payload 16 B. O tamanho faz
// versão futura falhar fechada; o CRC evita usar escrita parcial/corrompida.
class WeatherCacheCodec {
public:
    static constexpr size_t kBlobSize = 28;
    static utils::Status encode(const WeatherCacheEntry& entry, uint8_t* out, size_t capacity);
    static utils::Result<WeatherCacheEntry> decode(const uint8_t* data, size_t size);
};

// Política pura para teste: o timestamp UTC no blob mantém o throttle mesmo
// se a placa reiniciar entre duas consultas.
constexpr uint64_t kWeatherCacheMinWriteIntervalS = 30U * 60U;
bool should_save_weather_cache(uint64_t saved_utc_s, uint64_t now_utc_s);

class LittlefsWeatherCache final : public IWeatherCache {
public:
    utils::Result<WeatherCacheEntry> load() override;
    utils::Status save(const WeatherCacheEntry& entry) override;
};

// Monta `storage` em modo conservador. Só inicializa LittleFS quando o setor
// raiz está comprovadamente apagado (0xFF); corrupção nunca é formatada.
utils::Status initialize_weather_cache_storage();

}  // namespace cache
}  // namespace nova
