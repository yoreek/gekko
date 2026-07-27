#pragma once

#include "devices/display/lcd1602/Lcd1602Device.h"
#include "integrations/rest/hd44780/TypedHd44780DeviceApiAdapter.h"

namespace ewfm {

class Lcd1602DeviceApiAdapter final : public TypedHd44780DeviceApiAdapter<Lcd1602DeviceApiAdapter, Lcd1602Device, Lcd1602DeviceConfigV2> {
public:
    static constexpr const char* kTypeName = "lcd1602";
    static constexpr const char* kInvalidLayoutError = "lcd1602 layout is invalid";
    static constexpr const char* kLayoutSizeError = "lcd1602 layout exceeds supported size";
    static constexpr const char* kLayoutDependencyCountError = "lcd1602 layout exceeds supported dependency count";
    static constexpr const char* kLayoutPlaceholderError = "lcd1602 layout placeholder is invalid";
    static constexpr const char* kDepsRequiredError = "switch dependency is required";
    static constexpr const char* kDependencyCountError = "lcd1602 exceeds maximum dependency count";

    static const Lcd1602DeviceApiAdapter& instance() {
        static const Lcd1602DeviceApiAdapter adapter;
        return adapter;
    }

    static bool decodeConfig(const uint8_t* blob, size_t size, Lcd1602DeviceConfigV2& config) {
        return decodeLcd1602DeviceConfig(blob, size, config);
    }

    static const Hd44780ChannelConfigV1& channelsOf(const Lcd1602DeviceConfigV2& config) {
        return config.channels;
    }

    static constexpr DisplayLayoutProfile layoutProfile() {
        return characterCellDisplayLayoutProfile(16U, 2U, 0x01U);
    }
};

} // namespace ewfm
