# diag/ — diagnóstico e instrumentação de render da Onda 0 (não é produto).
# Pode tocar LVGL e board; nunca é dependência de board (sem ciclo).
idf_component_register(
    SRCS
        "src/render_metrics.cpp"
        "src/boot_report.cpp"
        "src/boot_diag.cpp"
        "src/render_probe.cpp"
        "src/render_workout.cpp"
        "src/flash_thrash.cpp"
    INCLUDE_DIRS "include"
    REQUIRES
        board
        esp_lvgl_adapter
        esp_psram
        esp_hw_support
        esp_timer
        esp_partition
        spi_flash
)
