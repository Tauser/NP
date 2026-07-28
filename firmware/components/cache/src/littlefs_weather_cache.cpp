#include "cache/weather_cache.hpp"

#include <cstdio>
#include <cstring>
#include <unistd.h>

#include "esp_littlefs.h"
#include "esp_partition.h"

namespace nova {
namespace cache {
namespace {
constexpr char kBasePath[] = "/cache";
constexpr char kPartitionLabel[] = "storage";
constexpr char kPath[] = "/cache/weather.v1";
constexpr char kTempPath[] = "/cache/weather.tmp";
constexpr size_t kBlankProbeBytes = 4096;

utils::Status read_exact(FILE* file, uint8_t* data, size_t size) {
    return std::fread(data, 1, size, file) == size && std::fgetc(file) == EOF ? utils::Status::kOk
                                                                                : utils::Status::kMalformed;
}

bool is_blank_storage_partition() {
    const esp_partition_t* partition = esp_partition_find_first(
        ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, kPartitionLabel);
    if (partition == nullptr) return false;
    uint8_t probe[kBlankProbeBytes] = {};
    if (esp_partition_read(partition, 0, probe, sizeof(probe)) != ESP_OK) return false;
    for (uint8_t byte : probe) {
        if (byte != 0xFFU) return false;
    }
    return true;
}

utils::Status mount_storage() {
    const esp_vfs_littlefs_conf_t config{
        .base_path = kBasePath,
        .partition_label = kPartitionLabel,
        .partition = nullptr,
        .format_if_mount_failed = false,
        .read_only = false,
        .dont_mount = false,
        .grow_on_mount = false,
    };
    return esp_vfs_littlefs_register(&config) == ESP_OK ? utils::Status::kOk : utils::Status::kInternal;
}
}  // namespace

utils::Status initialize_weather_cache_storage() {
    const utils::Status mounted = mount_storage();
    if (mounted == utils::Status::kOk || !is_blank_storage_partition()) return mounted;
    // Somente um setor raiz totalmente apagado pode ser inicializado sem risco
    // de descartar dado existente; mídia corrompida segue degradada e preservada.
    if (esp_littlefs_format(kPartitionLabel) != ESP_OK) return utils::Status::kInternal;
    return mount_storage();
}

utils::Result<WeatherCacheEntry> LittlefsWeatherCache::load() {
    FILE* file = std::fopen(kPath, "rb");
    if (file == nullptr) return utils::Result<WeatherCacheEntry>::fail(utils::Status::kNotFound);
    uint8_t blob[WeatherCacheCodec::kBlobSize] = {};
    const utils::Status read = read_exact(file, blob, sizeof(blob));
    std::fclose(file);
    return read == utils::Status::kOk ? WeatherCacheCodec::decode(blob, sizeof(blob))
                                      : utils::Result<WeatherCacheEntry>::fail(read);
}

utils::Status LittlefsWeatherCache::save(const WeatherCacheEntry& entry) {
    uint8_t blob[WeatherCacheCodec::kBlobSize] = {};
    const utils::Status encoded = WeatherCacheCodec::encode(entry, blob, sizeof(blob));
    if (encoded != utils::Status::kOk) return encoded;
    FILE* file = std::fopen(kTempPath, "wb");
    if (file == nullptr) return utils::Status::kInternal;
    const bool written = std::fwrite(blob, 1, sizeof(blob), file) == sizeof(blob) &&
                         std::fflush(file) == 0 && fsync(fileno(file)) == 0;
    std::fclose(file);
    if (!written) {
        std::remove(kTempPath);
        return utils::Status::kInternal;
    }
    return std::rename(kTempPath, kPath) == 0 ? utils::Status::kOk : utils::Status::kInternal;
}

}  // namespace cache
}  // namespace nova
