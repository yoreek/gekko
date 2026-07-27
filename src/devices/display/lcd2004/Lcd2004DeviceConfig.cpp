#include "devices/display/lcd2004/Lcd2004DeviceConfig.h"

#include <type_traits>

namespace ewfm {

static_assert(std::is_trivially_copyable<Lcd2004DeviceConfigV2>::value, "Lcd2004DeviceConfigV2 must be POD");
static_assert(sizeof(Lcd2004DeviceConfigV2::kMagic) - 1U + sizeof(Lcd2004DeviceConfigV2) <= kMaxDeviceConfigBytes,
              "Lcd2004DeviceConfigV2 exceeds device config bound");

bool decodeLcd2004DeviceConfig(const uint8_t* blob, size_t size, Lcd2004DeviceConfigV2& config) {
    return decodeHd44780DisplayDeviceConfig(blob, size, config);
}

} // namespace ewfm
