#pragma once

#include "devices/display/lcd2004_pin/Lcd2004PinDevice.h"
#include "integrations/rest/hd44780/TypedHd44780PinDeviceApiAdapter.h"

namespace ewfm {

class Lcd2004PinDeviceApiAdapter final
    : public TypedHd44780PinDeviceApiAdapter<Lcd2004PinDeviceApiAdapter, Lcd2004PinDevice, Lcd2004PinDeviceConfigV1> {
public:
    static constexpr const char* kTypeName = "lcd2004_pin";

    static const Lcd2004PinDeviceApiAdapter& instance() {
        static const Lcd2004PinDeviceApiAdapter adapter;
        return adapter;
    }

    static bool decodeConfig(const uint8_t* blob, size_t size, Lcd2004PinDeviceConfigV1& config) {
        return decodeLcd2004PinDeviceConfig(blob, size, config);
    }

    static constexpr DisplayLayoutProfile layoutProfile() {
        return characterCellDisplayLayoutProfile(20U, 4U, 0x01U);
    }
};

} // namespace ewfm
