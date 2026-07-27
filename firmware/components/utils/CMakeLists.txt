# utils/ — utilitários puros (docs/ARCHITECTURE.md §3).
# NÃO depende de nada: nem IDF, nem LVGL, nem outro componente nosso. É a base
# que `core`, `providers` e `cache` compartilham.
idf_component_register(
    SRCS
        "src/status.cpp"
        "src/http_client.cpp"
    INCLUDE_DIRS "include"
)
