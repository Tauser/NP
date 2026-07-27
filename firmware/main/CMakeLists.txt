idf_component_register(
    SRCS
        "app_main.cpp"
        "boot.cpp"
    INCLUDE_DIRS "."
    REQUIRES
        # main/ é wiring: monta e conecta (docs/ARCHITECTURE.md §3).
        board
        diag
        esp_lvgl_adapter
)
