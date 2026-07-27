# models/ — tipos de domínio puros (docs/ARCHITECTURE.md §3).
# NÃO depende de nada: nem IDF, nem LVGL, nem outro componente nosso.
# O arch_check verifica tanto a lista de REQUIRES quanto a ausência de includes
# de `esp_*`, `driver/` e `lvgl` aqui dentro.
idf_component_register(
    INCLUDE_DIRS "include"
)
