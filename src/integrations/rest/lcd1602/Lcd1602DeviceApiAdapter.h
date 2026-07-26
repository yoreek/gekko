#pragma once

#include "devices/display/lcd1602/Lcd1602Device.h"
#include "integrations/rest/hd44780/TypedHd44780DeviceApiAdapter.h"

namespace ewfm {

class Lcd1602DeviceApiAdapter final : public TypedHd44780DeviceApiAdapter<Lcd1602DeviceApiAdapter, Lcd1602Device, Lcd1602DeviceConfigV1> {
public:
    static constexpr const char* kTypeName = "lcd1602";
    static constexpr const char* kDepsRequiredError = "lcd1602 port expander dependency is required";
    static constexpr const char* kInvalidLineError = "lcd1602 line placeholder is invalid";
    static constexpr const char* kDependencyCountError = "lcd1602 exceeds maximum dependency count";

    static bool decodeConfig(const uint8_t* blob, size_t size, Lcd1602DeviceConfigV1& config) {
        return decodeLcd1602DeviceConfig(blob, size, config);
    }

    static uint8_t lineCount() {
        return 2U;
    }

    static const char* lineAt(const Lcd1602DeviceConfigV1& config, uint8_t index) {
        return index == 0U ? config.line1 : config.line2;
    }

    static const Hd44780ChannelConfigV1& channelsOf(const Lcd1602DeviceConfigV1& config) {
        return config.channels;
    }
};

} // namespace ewfm
