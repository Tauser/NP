# services/ — execução e regras de domínio. A rede real fica nesta borda; a
# política continua em core/ e os futuros providers continuam em interfaces.
idf_component_register(
    SRCS
        "src/clock_service.cpp"
        "src/sntp_service.cpp"
        "src/https_probe_service.cpp"
        "src/weather_service.cpp"
        "src/setup_service.cpp"
        "src/wifi_credentials_store.cpp"
        "src/wifi_credentials_store_mock.cpp"
        "src/wifi_provisioning_mailbox.cpp"
        "src/usb_wifi_provisioning_protocol.cpp"
        "src/usb_wifi_provisioner.cpp"
        "src/network_worker.cpp"
        "src/network_worker_task.cpp"
        "src/esp_http_client.cpp"
    INCLUDE_DIRS "include"
    REQUIRES
        board
        cache
        core
        utils
        providers
        esp_http_client
        esp-tls
        esp_timer
        esp_netif
        mbedtls
        nvs_flash
)
