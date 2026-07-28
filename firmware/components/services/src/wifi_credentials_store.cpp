#include "services/wifi_credentials_store.hpp"

#include <cstring>

#include "nvs.h"
#include "nvs_flash.h"

namespace nova {
namespace services {
namespace {
constexpr const char* kNamespace = "wifi_cfg";
constexpr const char* kSchemaKey = "schema";
constexpr const char* kSsidKey = "ssid";
constexpr const char* kPassphraseKey = "pass";
constexpr uint8_t kSchemaVersion = 1;

utils::Status status_from_nvs(esp_err_t error) {
    if (error == ESP_ERR_NVS_NOT_FOUND) return utils::Status::kNotFound;
    if (error == ESP_ERR_NVS_INVALID_LENGTH) return utils::Status::kMalformed;
    return error == ESP_OK ? utils::Status::kOk : utils::Status::kInternal;
}
}  // namespace

utils::Status initialize_wifi_credentials_storage() {
    return status_from_nvs(nvs_flash_init());
}

utils::Result<board::WifiCredentials> NvsWifiCredentialsStore::load() {
    nvs_handle_t handle;
    esp_err_t error = nvs_open(kNamespace, NVS_READONLY, &handle);
    if (error != ESP_OK) return utils::Result<board::WifiCredentials>::fail(status_from_nvs(error));
    uint8_t schema = 0;
    error = nvs_get_u8(handle, kSchemaKey, &schema);
    if (error == ESP_OK && schema != kSchemaVersion) {
        nvs_close(handle);
        // Versao futura e ignorada em vez de causar falha de boot ou escrita
        // destrutiva. A migracao explicita decidira o que fazer com ela.
        return utils::Result<board::WifiCredentials>::fail(utils::Status::kNotFound);
    }
    board::WifiCredentials credentials;
    size_t ssid_size = sizeof(credentials.ssid_);
    if (error == ESP_OK && schema == kSchemaVersion)
        error = nvs_get_str(handle, kSsidKey, credentials.ssid_, &ssid_size);
    size_t passphrase_size = sizeof(credentials.passphrase_);
    if (error == ESP_OK)
        error = nvs_get_str(handle, kPassphraseKey, credentials.passphrase_, &passphrase_size);
    nvs_close(handle);
    return error == ESP_OK ? utils::Result<board::WifiCredentials>::ok(credentials)
                           : utils::Result<board::WifiCredentials>::fail(status_from_nvs(error));
}

utils::Status NvsWifiCredentialsStore::save(const board::WifiCredentials& credentials) {
    nvs_handle_t handle = 0;
    esp_err_t error = nvs_open(kNamespace, NVS_READWRITE, &handle);
    if (error == ESP_OK) error = nvs_set_u8(handle, kSchemaKey, kSchemaVersion);
    if (error == ESP_OK) error = nvs_set_str(handle, kSsidKey, credentials.ssid_);
    if (error == ESP_OK) error = nvs_set_str(handle, kPassphraseKey, credentials.passphrase_);
    if (error == ESP_OK) error = nvs_commit(handle);
    if (handle != 0) nvs_close(handle);
    return status_from_nvs(error);
}

}  // namespace services
}  // namespace nova
