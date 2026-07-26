#pragma once

#include "devices/display/lcd2004/Lcd2004Device.h"
#include "integrations/rest/hd44780/TypedHd44780DeviceApiAdapter.h"

namespace ewfm {

class Lcd2004DeviceApiAdapter final : public TypedHd44780DeviceApiAdapter<Lcd2004DeviceApiAdapter, Lcd2004Device, Lcd2004DeviceConfigV1> {
public:
    static constexpr const char* kTypeName = "lcd2004";
    static constexpr const char* kDepsRequiredError = "lcd2004 port expander dependency is required";
    static constexpr const char* kInvalidLineError = "lcd2004 line placeholder is invalid";
    static constexpr const char* kDependencyCountError = "lcd2004 exceeds maximum dependency count";

    static bool decodeConfig(const uint8_t* blob, size_t size, Lcd2004DeviceConfigV1& config) {
        return decodeLcd2004DeviceConfig(blob, size, config);
    }

    static uint8_t lineCount() {
        return 4U;
    }

    static const char* lineAt(const Lcd2004DeviceConfigV1& config, uint8_t index) {
        switch (index) {
        case 0U:
            return config.line1;
        case 1U:
            return config.line2;
        case 2U:
            return config.line3;
        default:
            return config.line4;
        }
    }

    static const Hd44780ChannelConfigV1& channelsOf(const Lcd2004DeviceConfigV1& config) {
        return config.channels;
    }
};

} // namespace ewfm
