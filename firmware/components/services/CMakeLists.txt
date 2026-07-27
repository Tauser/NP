# services/ — execução e regras de domínio. A rede real fica nesta borda; a
# política continua em core/ e os futuros providers continuam em interfaces.
idf_component_register(
    SRCS
        "src/network_worker.cpp"
        "src/network_worker_task.cpp"
        "src/esp_http_client.cpp"
    INCLUDE_DIRS "include"
    REQUIRES
        core
        utils
        esp_http_client
        esp-tls
        esp_timer
        mbedtls
)
