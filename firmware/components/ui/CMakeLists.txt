# ui/ — shell e registro de telas. Só depende de core/models/LVGL; não conhece
# services, providers, board ou rede (ARCHITECTURE §3, imposto por arch_check).
idf_component_register(
    SRCS
        "src/screen_registry.cpp"
        "src/screen_catalog.cpp"
        "src/shell.cpp"
    INCLUDE_DIRS "include"
    REQUIRES
        core
        models
        lvgl
)
