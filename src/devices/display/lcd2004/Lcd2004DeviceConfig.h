#pragma once

#include "devices/display/hd44780/Hd44780DisplayDeviceConfigBase.h"

#include <cstddef>
#include <cstdint>

namespace ewfm {

#pragma pack(push, 1)
struct Lcd2004DeviceConfigV2 : Hd44780DisplayDeviceConfigBase<Lcd2004DeviceConfigV2> {
    static constexpr char kMagic[] = "LCD2004-2";
};
#pragma pack(pop)

using Lcd2004DeviceConfigV1 = Lcd2004DeviceConfigV2;

constexpr size_t lcd2004DeviceConfigSize(const Lcd2004DeviceConfigV2&) {
    return sizeof(Lcd2004DeviceConfigV2::kMagic) - 1U + sizeof(Lcd2004DeviceConfigV2);
}

bool decodeLcd2004DeviceConfig(const uint8_t* blob, size_t size, Lcd2004DeviceConfigV2& config);

} // namespace ewfm
