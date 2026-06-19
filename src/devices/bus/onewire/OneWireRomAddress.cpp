#include "devices/bus/onewire/OneWireRomAddress.h"

#include <cctype>
#include <cstring>

namespace ewfm {

namespace {
constexpr char kHexDigits[] = "0123456789ABCDEF";

int hexValue(const char c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'a' && c <= 'f') {
        return 10 + (c - 'a');
    }
    if (c >= 'A' && c <= 'F') {
        return 10 + (c - 'A');
    }
    return -1;
}
} // namespace

bool formatOneWireRomAddress(const OneWireRomAddress& address, char (&out)[17]) {
    for (size_t index = 0; index < 8; ++index) {
        out[index * 2] = kHexDigits[(address.bytes[index] >> 4) & 0x0F];
        out[index * 2 + 1] = kHexDigits[address.bytes[index] & 0x0F];
    }
    out[16] = '\0';
    return true;
}

bool parseOneWireRomAddress(const char* input, OneWireRomAddress& address) {
    if (input == nullptr) {
        return false;
    }
    if (std::strlen(input) != 16U) {
        return false;
    }

    for (size_t index = 0; index < 8; ++index) {
        const int high = hexValue(input[index * 2]);
        const int low = hexValue(input[index * 2 + 1]);
        if (high < 0 || low < 0) {
            return false;
        }
        address.bytes[index] = static_cast<uint8_t>((high << 4) | low);
    }

    return true;
}

uint8_t oneWireCrc8(const uint8_t* data, size_t len) {
    if (data == nullptr) {
        return 0;
    }

    uint8_t crc = 0;
    for (size_t index = 0; index < len; ++index) {
        uint8_t inbyte = data[index];
        for (uint8_t bit = 0; bit < 8; ++bit) {
            const uint8_t mix = static_cast<uint8_t>((crc ^ inbyte) & 0x01U);
            crc >>= 1U;
            if (mix != 0U) {
                crc ^= 0x8CU;
            }
            inbyte >>= 1U;
        }
    }
    return crc;
}

bool oneWireRomCrcValid(const OneWireRomAddress& address) {
    return oneWireCrc8(address.bytes, 7U) == address.bytes[7];
}

bool oneWireRomCrcValid(const IOneWireBusDriver& driver, const OneWireRomAddress& address) {
    return driver.crc8(address.bytes, 7U) == address.bytes[7];
}

} // namespace ewfm
