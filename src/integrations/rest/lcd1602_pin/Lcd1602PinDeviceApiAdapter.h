#pragma once

#include "devices/display/lcd1602_pin/Lcd1602PinDevice.h"
#include "integrations/rest/hd44780/TypedHd44780PinDeviceApiAdapter.h"

namespace ewfm {

class Lcd1602PinDeviceApiAdapter final
    : public TypedHd44780PinDeviceApiAdapter<Lcd1602PinDeviceApiAdapter, Lcd1602PinDevice, Lcd1602PinDeviceConfigV1> {
public:
    static constexpr const char* kTypeName = "lcd1602_pin";

    static const Lcd1602PinDeviceApiAdapter& instance() {
        static const Lcd1602PinDeviceApiAdapter adapter;
        return adapter;
    }

    static bool decodeConfig(const uint8_t* blob, size_t size, Lcd1602PinDeviceConfigV1& config) {
        return decodeLcd1602PinDeviceConfig(blob, size, config);
    }

    static constexpr DisplayLayoutProfile layoutProfile() {
        return characterCellDisplayLayoutProfile(16U, 2U, 0x01U);
    }
};

} // namespace ewfm
