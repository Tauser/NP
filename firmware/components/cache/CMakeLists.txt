# cache/ — blobs offline versionados. Nao depende de service/UI; LittleFS e so
# a borda de armazenamento deste componente.
idf_component_register(
    SRCS
        "src/weather_cache_codec.cpp"
        "src/littlefs_weather_cache.cpp"
    INCLUDE_DIRS "include"
    REQUIRES
        models
        utils
        esp_partition
        joltwallet__littlefs
)
