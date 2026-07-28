# providers/ — contratos e adaptadores de APIs externas (ARCHITECTURE §3).
# Pode depender apenas de modelos e utilitários; StateStore, UI, HAL e agenda
# de requests pertencem às outras camadas e são vetados pelo arch_check.
idf_component_register(
    SRCS
        "src/mock_time_provider.cpp"
        "src/open_meteo_weather_provider.cpp"
    INCLUDE_DIRS "include"
    REQUIRES
        models
        utils
)
