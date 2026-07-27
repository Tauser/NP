cmake_minimum_required(VERSION 3.16)

# Modo "torture" (docs/GLITCH-PROTOCOLO.md §3.2): só render, sem rede/NVS/cache,
# churn a ~10 Hz para maximizar flush. Uma VARIÁVEL por experimento (§5):
#   idf.py -DNOVA_TORTURE=ON build   → build de bisseção
#   idf.py build                     → modo ocioso (~1 Hz)
option(NOVA_TORTURE "Build torture render-only mode (GLITCH-PROTOCOLO §3.2)" OFF)
if(NOVA_TORTURE)
    add_compile_definitions(NOVA_TORTURE=1)
    message(STATUS "NovaPanel: MODO TORTURE ligado")
endif()

# Modo "clareza": conteúdo grande, mais lento e rotulado, para desambiguar a
# olho/foto o que é conteúdo de diagnóstico do que seria o glitch. Use o overlay
# sdkconfig.clarity junto (fonte grande). Render permanece agressivo o bastante
# para pressionar flush, mas legível.
option(NOVA_CLARITY "Build clareza: carimbo grande/legível para desambiguação" OFF)
if(NOVA_CLARITY)
    add_compile_definitions(NOVA_CLARITY=1)
    message(STATUS "NovaPanel: MODO CLAREZA ligado")
endif()

# Bisseção §3.2 / hipótese 2.5: erase de flash durante o render, para testar
# contenção no barramento MSPI (RESOURCE-BUDGET §1). Diagnóstico, nunca produção.
option(NOVA_FLASH_THRASH "Erase de flash periodico durante render (GLITCH §3.2, hip. 2.5)" OFF)
if(NOVA_FLASH_THRASH)
    add_compile_definitions(NOVA_FLASH_THRASH=1)
    message(STATUS "NovaPanel: FLASH-THRASH ligado (diagnostico)")
endif()

# Bisseção do artefato do carimbo: congela o texto do contador (a barra continua
# piscando). Isola "atualizar a região" de "renderizar a região" (GLITCH §2.3).
option(NOVA_STATIC_STAMP "Carimbo estatico, para isolar tearing de update" OFF)
if(NOVA_STATIC_STAMP)
    add_compile_definitions(NOVA_STATIC_STAMP=1)
    message(STATUS "NovaPanel: CARIMBO ESTATICO (bisseccao)")
endif()

# Hipotese de MISS DE CACHE DE GLIFO (RESOURCE-BUDGET §1.2): fonte pequena no
# carimbo = menos bytes de flash por digito novo = menos contencao de MSPI.
# Confirmada em bancada: reduzir a fonte diminui o artefato (ADR-025).
option(NOVA_SMALL_STAMP "Carimbo com fonte pequena (hipotese de glifo/flash)" OFF)
if(NOVA_SMALL_STAMP)
    add_compile_definitions(NOVA_SMALL_STAMP=1)
    message(STATUS "NovaPanel: CARIMBO FONTE PEQUENA (hipotese glifo)")
endif()

# NOTA: os flags NOVA_DOUBLE_BUFFER, NOVA_NO_ROTATE e NOVA_AVOID_TEARING foram
# REMOVIDOS. Eram experimentos sobre o esp_lvgl_port, que deixou de ser o backend
# de display (ADR-026). Rotacao e anti-tearing agora sao configuracao do
# esp_lvgl_adapter, nao flags de build.

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(novapanel)
