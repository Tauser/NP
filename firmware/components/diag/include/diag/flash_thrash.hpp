// Experimento de bisseção (docs/GLITCH-PROTOCOLO.md §3.2, hipótese 2.5).
//
// Religa UM subsistema — erase de flash — por cima do render torture, para
// testar se o artefato acompanha a atividade de erase no barramento MSPI
// compartilhado com a PSRAM (RESOURCE-BUDGET §1: "artefato a cada nvs_commit /
// escrita LittleFS"). Cada erase é logado com duração, para casar com o carimbo
// de frame no vídeo (§3.1). Alvo-only, atrás do flag de build NOVA_FLASH_THRASH.
#pragma once

#include <cstdint>

namespace nova {
namespace diag {

// Cria uma task que apaga um setor de 4 KB da partição `storage` (não usada
// nesta baseline) a cada `period_ms`. Não faz nada se a partição não existir.
void start_flash_thrash(uint32_t period_ms);

}  // namespace diag
}  // namespace nova
