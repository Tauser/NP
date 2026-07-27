# NovaPanel v5 -- referencia de design.
#
# NAO faz parte do build do produto: nao existe nenhum add_subdirectory
# apontando para ca a partir de firmware/. Este arquivo existe para quem
# quiser compilar os modelos contra um LVGL de simulador (SDL/Wayland) e
# ver as telas fora da placa.
#
# O firmware compila as telas de verdade por
# firmware/components/ui/CMakeLists.txt.

cmake_minimum_required(VERSION 3.16)
project(novapanel_v5_reference C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_library(novapanel_v5 STATIC
    core/np_styles.c
    core/np_components.c
    screens/np_boot.c
    screens/np_home.c
    screens/np_market.c
    screens/np_setup.c
    screens/np_weather.c
    screens/np_timer.c
    screens/np_agenda.c
    screens/np_alarms.c
    screens/np_notifications.c
    screens/np_devices.c
    screens/np_settings.c
    screens/np_sheets.c
    screens/np_catalog.c
)

target_include_directories(novapanel_v5 PUBLIC core screens)

# Espera um alvo `lvgl` ja definido pelo projeto que inclui este diretorio.
if(TARGET lvgl)
    target_link_libraries(novapanel_v5 PUBLIC lvgl)
else()
    message(WARNING
        "novapanel_v5: alvo `lvgl` nao encontrado. "
        "Esta pasta e referencia; forneca o LVGL para compilar de fato.")
endif()

target_compile_options(novapanel_v5 PRIVATE -Wall -Wextra)
