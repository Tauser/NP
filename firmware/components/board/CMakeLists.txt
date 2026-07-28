# board/ — HAL (docs/ARCHITECTURE.md §3/§4). Depende SÓ de BSP/drivers e do
# pipeline gráfico; nunca de core/services/ui (imposto por arch_check.sh).
idf_component_register(
    SRCS
        "src/dma_safety.cpp"
        "src/dma_safe_assert.cpp"
        "src/waveshare_board.cpp"
    INCLUDE_DIRS "include"
    REQUIRES
        # Nome do COMPONENTE é esp_lvgl_adapter; os headers usam o prefixo
        # esp_lv_adapter_*. Não confundir os dois.
        esp_lvgl_adapter
        esp32_p4_wifi6_touch_lcd_7b
        esp_hosted
        esp_event
        esp_netif
        esp_wifi
)
