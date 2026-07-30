#include "devices/display/lcd1602_pin/Lcd1602PinDeviceConfig.h"

#include <type_traits>

namespace ewfm {

static_assert(std::is_trivially_copyable<Lcd1602PinDeviceConfigV1>::value, "Lcd1602PinDeviceConfigV1 must be POD");
static_assert(sizeof(Lcd1602PinDeviceConfigV1::kMagic) - 1U + sizeof(Lcd1602PinDeviceConfigV1) <= kMaxDeviceConfigBytes,
              "Lcd1602PinDeviceConfigV1 exceeds device config bound");

bool decodeLcd1602PinDeviceConfig(const uint8_t* blob, size_t size, Lcd1602PinDeviceConfigV1& config) {
    return decodeHd44780PinDisplayDeviceConfig(blob, size, config);
}

} // namespace ewfm
