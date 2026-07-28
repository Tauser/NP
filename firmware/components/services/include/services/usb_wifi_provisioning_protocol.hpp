// Protocolo local de provisionamento, transportado apenas sobre USB fisico.
// Frame ASCII: "NPW1 <base64-ssid> <base64-senha>\n". Base64 evita ambiguidade
// de espacos, mas NAO e criptografia: a garantia de confidencialidade aqui e o
// cabo USB fisico, dentro do modelo de ameaca do produto.
#pragma once

#include <cstddef>

#include "board/i_board.hpp"
#include "utils/status.hpp"

namespace nova {
namespace services {

utils::Status parse_usb_wifi_provisioning_frame(const char* frame, size_t length,
                                                 board::WifiCredentials& credentials);

}  // namespace services
}  // namespace nova
