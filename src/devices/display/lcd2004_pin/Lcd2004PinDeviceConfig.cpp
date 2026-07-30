#include "devices/display/lcd2004_pin/Lcd2004PinDeviceConfig.h"

#include <type_traits>

namespace ewfm {

static_assert(std::is_trivially_copyable<Lcd2004PinDeviceConfigV1>::value, "Lcd2004PinDeviceConfigV1 must be POD");
static_assert(sizeof(Lcd2004PinDeviceConfigV1::kMagic) - 1U + sizeof(Lcd2004PinDeviceConfigV1) <= kMaxDeviceConfigBytes,
              "Lcd2004PinDeviceConfigV1 exceeds device config bound");

bool decodeLcd2004PinDeviceConfig(const uint8_t* blob, size_t size, Lcd2004PinDeviceConfigV1& config) {
    return decodeHd44780PinDisplayDeviceConfig(blob, size, config);
}

} // namespace ewfm
