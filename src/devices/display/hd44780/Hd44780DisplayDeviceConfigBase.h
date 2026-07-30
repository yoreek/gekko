#pragma once

#include "devices/bus/i2c/I2cDeviceConfig.h"
#include "devices/core/ConfigCodec.h"

#include <cstddef>

namespace ewfm {

// HD44780 character displays are driven through an embedded PCF8574 I2C backpack (the
// near-universal real-world form factor for LCD1602/2004 modules - see Pcf857xIoDriver, shared
// with the standalone PCF857x expander device). Direct-GPIO wiring is a separate device family
// (Hd44780PinDisplayDeviceConfigBase); this base is I2C-only, so channels are fixed to PCF8574's
// 8 bit positions (0-7), not the 16 PCF8575 would offer.
constexpr uint8_t kHd44780I2cMaxChannel = 7U;
// Sentinel for backlightChannel meaning "not wired" -- the backlight line is the one optional
// channel (some boards tie it permanently high instead of giving it a PCF8574 pin).
constexpr uint8_t kHd44780ChannelUnset = 0xFFU;

#pragma pack(push, 1)
template <typename Derived> struct Hd44780DisplayDeviceConfigBase : I2cDeviceConfigV1 {
    Hd44780DisplayDeviceConfigBase() : I2cDeviceConfigV1(0x27U) {}

    // PCF8574 bit positions (0-7), not dependency-slot indices. Defaults match the wiring
    // convention nearly every off-the-shelf LCM1602/2004-IIC backpack uses.
    uint8_t rsChannel{0U};
    uint8_t eChannel{2U};
    uint8_t d4Channel{4U};
    uint8_t d5Channel{5U};
    uint8_t d6Channel{6U};
    uint8_t d7Channel{7U};
    uint8_t backlightChannel{3U}; // kHd44780ChannelUnset = not wired

    DeviceValidationResult validate() const {
        const DeviceValidationResult baseResult = I2cDeviceConfigV1::validate();
        if (!baseResult.ok()) {
            return baseResult;
        }
        if (rsChannel > kHd44780I2cMaxChannel || eChannel > kHd44780I2cMaxChannel || d4Channel > kHd44780I2cMaxChannel ||
            d5Channel > kHd44780I2cMaxChannel || d6Channel > kHd44780I2cMaxChannel || d7Channel > kHd44780I2cMaxChannel) {
            return {DeviceError::InvalidConfig, "hd44780 channel is out of range"};
        }
        if (backlightChannel != kHd44780ChannelUnset && backlightChannel > kHd44780I2cMaxChannel) {
            return {DeviceError::InvalidConfig, "hd44780 backlight channel is out of range"};
        }
        const uint8_t channels[7] = {rsChannel, eChannel, d4Channel, d5Channel, d6Channel, d7Channel, backlightChannel};
        for (size_t i = 0; i < 7U; ++i) {
            if (channels[i] == kHd44780ChannelUnset) {
                continue;
            }
            for (size_t j = i + 1U; j < 7U; ++j) {
                if (channels[j] != kHd44780ChannelUnset && channels[i] == channels[j]) {
                    return {DeviceError::InvalidConfig, "hd44780 channels must be distinct"};
                }
            }
        }
        return {};
    }

    bool parseJson(const JsonObjectConst& input, const char*& error) {
        if (!I2cDeviceConfigV1::parseJson(input, error)) {
            return false;
        }
        rsChannel = static_cast<uint8_t>(input["rsChannel"] | static_cast<int>(rsChannel));
        eChannel = static_cast<uint8_t>(input["eChannel"] | static_cast<int>(eChannel));
        d4Channel = static_cast<uint8_t>(input["d4Channel"] | static_cast<int>(d4Channel));
        d5Channel = static_cast<uint8_t>(input["d5Channel"] | static_cast<int>(d5Channel));
        d6Channel = static_cast<uint8_t>(input["d6Channel"] | static_cast<int>(d6Channel));
        d7Channel = static_cast<uint8_t>(input["d7Channel"] | static_cast<int>(d7Channel));
        backlightChannel = static_cast<uint8_t>(input["backlightChannel"] | static_cast<int>(backlightChannel));
        return true;
    }

    void writeJson(JsonObject output) const {
        I2cDeviceConfigV1::writeJson(output);
        output["rsChannel"] = rsChannel;
        output["eChannel"] = eChannel;
        output["d4Channel"] = d4Channel;
        output["d5Channel"] = d5Channel;
        output["d6Channel"] = d6Channel;
        output["d7Channel"] = d7Channel;
        output["backlightChannel"] = backlightChannel;
    }
};
#pragma pack(pop)

template <typename Config> constexpr size_t hd44780DisplayDeviceConfigSize(const Config&) {
    return sizeof(Config::kMagic) - 1U + sizeof(Config);
}

template <typename Config> bool decodeHd44780DisplayDeviceConfig(const uint8_t* blob, size_t size, Config& config) {
    return decodeValidatedFixedConfigBlob(Config::kMagic, blob, size, config);
}

} // namespace ewfm
