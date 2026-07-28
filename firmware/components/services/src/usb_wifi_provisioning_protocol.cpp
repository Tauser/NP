#include "services/usb_wifi_provisioning_protocol.hpp"

#include <cstring>

namespace nova {
namespace services {
namespace {

int base64_value(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return -1;
}

bool decode_base64(const char* encoded, size_t encoded_length, char* output, size_t output_capacity) {
    if (encoded_length == 0 || encoded_length % 4 != 0 || output_capacity == 0) return false;
    size_t written = 0;
    for (size_t index = 0; index < encoded_length; index += 4) {
        const char c0 = encoded[index];
        const char c1 = encoded[index + 1];
        const char c2 = encoded[index + 2];
        const char c3 = encoded[index + 3];
        const int a = base64_value(c0);
        const int b = base64_value(c1);
        const int c = c2 == '=' ? 0 : base64_value(c2);
        const int d = c3 == '=' ? 0 : base64_value(c3);
        const bool pad2 = c2 == '=';
        const bool pad3 = c3 == '=';
        if (a < 0 || b < 0 || c < 0 || d < 0 || (pad2 && !pad3) ||
            ((pad2 || pad3) && index + 4 != encoded_length)) {
            return false;
        }
        if (written + 1 >= output_capacity) return false;
        output[written++] = static_cast<char>((a << 2) | (b >> 4));
        if (!pad2) {
            if (written + 1 >= output_capacity) return false;
            output[written++] = static_cast<char>((b << 4) | (c >> 2));
        }
        if (!pad3) {
            if (written + 1 >= output_capacity) return false;
            output[written++] = static_cast<char>((c << 6) | d);
        }
    }
    output[written] = '\0';
    return true;
}

bool is_space(char c) { return c == ' '; }
}  // namespace

utils::Status parse_usb_wifi_provisioning_frame(const char* frame, size_t length,
                                                 board::WifiCredentials& credentials) {
    constexpr char kPrefix[] = "NPW1 ";
    if (frame == nullptr || length <= sizeof(kPrefix) ||
        std::memcmp(frame, kPrefix, sizeof(kPrefix) - 1) != 0) {
        return utils::Status::kMalformed;
    }
    const size_t ssid_start = sizeof(kPrefix) - 1;
    size_t separator = ssid_start;
    while (separator < length && !is_space(frame[separator])) ++separator;
    if (separator == ssid_start || separator == length) return utils::Status::kMalformed;
    const size_t passphrase_start = separator + 1;
    if (passphrase_start >= length) return utils::Status::kMalformed;
    size_t passphrase_end = passphrase_start;
    while (passphrase_end < length && !is_space(frame[passphrase_end])) ++passphrase_end;
    if (passphrase_end != length) return utils::Status::kMalformed;

    board::WifiCredentials decoded;
    if (!decode_base64(frame + ssid_start, separator - ssid_start, decoded.ssid_,
                       sizeof(decoded.ssid_)) ||
        !decode_base64(frame + passphrase_start, passphrase_end - passphrase_start,
                       decoded.passphrase_, sizeof(decoded.passphrase_))) {
        return utils::Status::kMalformed;
    }
    credentials = decoded;
    decoded = {};
    return utils::Status::kOk;
}

}  // namespace services
}  // namespace nova
