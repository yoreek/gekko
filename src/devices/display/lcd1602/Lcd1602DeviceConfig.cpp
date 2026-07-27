#include "devices/display/lcd1602/Lcd1602DeviceConfig.h"

#include <type_traits>

namespace ewfm {

static_assert(std::is_trivially_copyable<Lcd1602DeviceConfigV2>::value, "Lcd1602DeviceConfigV2 must be POD");
static_assert(sizeof(Lcd1602DeviceConfigV2::kMagic) - 1U + sizeof(Lcd1602DeviceConfigV2) <= kMaxDeviceConfigBytes,
              "Lcd1602DeviceConfigV2 exceeds device config bound");

bool decodeLcd1602DeviceConfig(const uint8_t* blob, size_t size, Lcd1602DeviceConfigV2& config) {
    return decodeHd44780DisplayDeviceConfig(blob, size, config);
}

} // namespace ewfm
