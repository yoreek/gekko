#pragma once

#include "devices/display/hd44780/Hd44780PinDisplayDeviceConfigBase.h"

#include <cstddef>
#include <cstdint>

namespace ewfm {

#pragma pack(push, 1)
struct Lcd2004PinDeviceConfigV1 : Hd44780PinDisplayDeviceConfigBase<Lcd2004PinDeviceConfigV1> {
    static constexpr char kMagic[] = "LCD2004PIN-1";
};
#pragma pack(pop)

constexpr size_t lcd2004PinDeviceConfigSize(const Lcd2004PinDeviceConfigV1&) {
    return sizeof(Lcd2004PinDeviceConfigV1::kMagic) - 1U + sizeof(Lcd2004PinDeviceConfigV1);
}

bool decodeLcd2004PinDeviceConfig(const uint8_t* blob, size_t size, Lcd2004PinDeviceConfigV1& config);

} // namespace ewfm
