#pragma once

#include "devices/display/hd44780/Hd44780PinDisplayDeviceConfigBase.h"

#include <cstddef>
#include <cstdint>

namespace ewfm {

#pragma pack(push, 1)
struct Lcd1602PinDeviceConfigV1 : Hd44780PinDisplayDeviceConfigBase<Lcd1602PinDeviceConfigV1> {
    static constexpr char kMagic[] = "LCD1602PIN-1";
};
#pragma pack(pop)

constexpr size_t lcd1602PinDeviceConfigSize(const Lcd1602PinDeviceConfigV1&) {
    return sizeof(Lcd1602PinDeviceConfigV1::kMagic) - 1U + sizeof(Lcd1602PinDeviceConfigV1);
}

bool decodeLcd1602PinDeviceConfig(const uint8_t* blob, size_t size, Lcd1602PinDeviceConfigV1& config);

} // namespace ewfm
