# core/ — núcleo de estado, eventos e intenções (docs/ARCHITECTURE.md §3).
# Só pode depender de `models` e `utils` (imposto por arch_check.sh).
# NÃO conhece IDF, LVGL nem a HAL: o lock é injetado, o que mantém tudo
# testável no host.
idf_component_register(
    SRCS
        "src/event_bus.cpp"
        "src/state_store.cpp"
        "src/action_queue.cpp"
    INCLUDE_DIRS "include"
    REQUIRES
        models
)
