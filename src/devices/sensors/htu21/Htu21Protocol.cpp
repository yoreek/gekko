#include "devices/sensors/htu21/Htu21Protocol.h"

namespace ewfm {

namespace {
constexpr uint32_t kHtu21CrcPoly = 0x988000UL;
constexpr uint16_t kHtu21StatusBitsMask = 0xFFFC;
} // namespace

bool htu21CrcValid(uint16_t rawValue, uint8_t crc) {
    uint32_t remainder = static_cast<uint32_t>(rawValue) << 8;
    remainder |= crc;

    uint32_t divisor = kHtu21CrcPoly;
    for (int bit = 0; bit < 16; ++bit) {
        if ((remainder & (1UL << (23 - bit))) != 0UL) {
            remainder ^= divisor;
        }
        divisor >>= 1;
    }
    return remainder == 0UL;
}

int32_t htu21RawToMilliCelsius(uint16_t raw) {
    const uint16_t masked = raw & kHtu21StatusBitsMask;
    return static_cast<int32_t>((static_cast<int64_t>(masked) * 175720) / 65536) - 46850;
}

int32_t htu21RawToMilliPercent(uint16_t raw) {
    const uint16_t masked = raw & kHtu21StatusBitsMask;
    const int32_t milliPercent = static_cast<int32_t>((static_cast<int64_t>(masked) * 125000) / 65536) - 6000;
    if (milliPercent < 0) {
        return 0;
    }
    if (milliPercent > 100000) {
        return 100000;
    }
    return milliPercent;
}

} // namespace ewfm
