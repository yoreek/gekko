#pragma once

#include "devices/display/lcd2004/Lcd2004Device.h"
#include "integrations/rest/hd44780/TypedHd44780DeviceApiAdapter.h"

namespace ewfm {

class Lcd2004DeviceApiAdapter final : public TypedHd44780DeviceApiAdapter<Lcd2004DeviceApiAdapter, Lcd2004Device, Lcd2004DeviceConfigV2> {
public:
    static constexpr const char* kTypeName = "lcd2004";
    static constexpr const char* kInvalidLayoutError = "lcd2004 layout is invalid";
    static constexpr const char* kLayoutSizeError = "lcd2004 layout exceeds supported size";
    static constexpr const char* kLayoutDependencyCountError = "lcd2004 layout exceeds supported dependency count";
    static constexpr const char* kLayoutPlaceholderError = "lcd2004 layout placeholder is invalid";
    static constexpr const char* kDepsRequiredError = "switch dependency is required";
    static constexpr const char* kDependencyCountError = "lcd2004 exceeds maximum dependency count";

    static const Lcd2004DeviceApiAdapter& instance() {
        static const Lcd2004DeviceApiAdapter adapter;
        return adapter;
    }

    static bool decodeConfig(const uint8_t* blob, size_t size, Lcd2004DeviceConfigV2& config) {
        return decodeLcd2004DeviceConfig(blob, size, config);
    }

    static const Hd44780ChannelConfigV1& channelsOf(const Lcd2004DeviceConfigV2& config) {
        return config.channels;
    }

    static constexpr DisplayLayoutProfile layoutProfile() {
        return characterCellDisplayLayoutProfile(20U, 4U, 0x01U);
    }
};

} // namespace ewfm
