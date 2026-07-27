#pragma once

#include "devices/display/hd44780/Hd44780DisplayDeviceConfigBase.h"

#include <cstddef>
#include <cstdint>

namespace ewfm {

#pragma pack(push, 1)
struct Lcd1602DeviceConfigV2 : Hd44780DisplayDeviceConfigBase<Lcd1602DeviceConfigV2> {
    static constexpr char kMagic[] = "LCD1602-2";
};
#pragma pack(pop)

using Lcd1602DeviceConfigV1 = Lcd1602DeviceConfigV2;

constexpr size_t lcd1602DeviceConfigSize(const Lcd1602DeviceConfigV2&) {
    return sizeof(Lcd1602DeviceConfigV2::kMagic) - 1U + sizeof(Lcd1602DeviceConfigV2);
}

bool decodeLcd1602DeviceConfig(const uint8_t* blob, size_t size, Lcd1602DeviceConfigV2& config);

} // namespace ewfm
