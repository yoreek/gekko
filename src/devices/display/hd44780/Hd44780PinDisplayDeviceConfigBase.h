#pragma once

#include "devices/core/ConfigCodec.h"
#include "devices/core/DeviceBaseConfig.h"
#include "devices/switch/gpio/GpioSwitchDeviceConfig.h"

#include <cstddef>

namespace ewfm {

// HD44780 character displays wired directly to ESP32 GPIOs -- no I2C, no dependency at all beyond
// the optional layout-derived MetricSource list (mirrors Tm1637Device/Ds1302RtcDevice: the display
// owns its pins outright). Sibling of Hd44780DisplayDeviceConfigBase, which is the I2C/PCF8574
// variant. Uses the project-wide kGpioPinUnset sentinel (DeviceTypes.h) for all 7 pin fields.

#pragma pack(push, 1)
template <typename Derived> struct Hd44780PinDisplayDeviceConfigBase : DeviceBaseConfigV1 {
    // No real defaults exist for 6 independently-wired data/control lines, and defaulting them all
    // to the same value (formerly 0) meant a freshly-created device failed the "pins must be
    // distinct" check below on its own compiled-in defaults. 0xFF forces the user to pick 6 real,
    // distinct pins explicitly -- matches the SPA's lcd1602-pin.ts/lcd2004-pin.ts, which already
    // default here to LCD1602_PIN_UNSET/LCD2004_PIN_UNSET (255). See
    // docs/pin-configuration-conventions.md.
    uint8_t rsPin{kGpioPinUnset};
    uint8_t ePin{kGpioPinUnset};
    uint8_t d4Pin{kGpioPinUnset};
    uint8_t d5Pin{kGpioPinUnset};
    uint8_t d6Pin{kGpioPinUnset};
    uint8_t d7Pin{kGpioPinUnset};
    uint8_t backlightPin{kGpioPinUnset};

    DeviceValidationResult validate() const {
        const DeviceValidationResult baseResult = DeviceBaseConfigV1::validate();
        if (!baseResult.ok()) {
            return baseResult;
        }
        if (!gpioSwitchPinIsValid(rsPin) || !gpioSwitchPinIsValid(ePin) || !gpioSwitchPinIsValid(d4Pin) || !gpioSwitchPinIsValid(d5Pin) ||
            !gpioSwitchPinIsValid(d6Pin) || !gpioSwitchPinIsValid(d7Pin)) {
            return {DeviceError::InvalidConfig, "hd44780 pin is invalid"};
        }
        if (backlightPin != kGpioPinUnset && !gpioSwitchPinIsValid(backlightPin)) {
            return {DeviceError::InvalidConfig, "hd44780 backlight pin is invalid"};
        }
        const uint8_t pins[7] = {rsPin, ePin, d4Pin, d5Pin, d6Pin, d7Pin, backlightPin};
        for (size_t i = 0; i < 7U; ++i) {
            if (pins[i] == kGpioPinUnset) {
                continue;
            }
            for (size_t j = i + 1U; j < 7U; ++j) {
                if (pins[j] != kGpioPinUnset && pins[i] == pins[j]) {
                    return {DeviceError::InvalidConfig, "hd44780 pins must be distinct"};
                }
            }
        }
        return {};
    }

    bool parseJson(const JsonObjectConst& input, const char*& error) {
        if (!parseDeviceBaseConfigJson(input, *this, error)) {
            return false;
        }
        rsPin = static_cast<uint8_t>(input["rsPin"] | static_cast<int>(rsPin));
        ePin = static_cast<uint8_t>(input["ePin"] | static_cast<int>(ePin));
        d4Pin = static_cast<uint8_t>(input["d4Pin"] | static_cast<int>(d4Pin));
        d5Pin = static_cast<uint8_t>(input["d5Pin"] | static_cast<int>(d5Pin));
        d6Pin = static_cast<uint8_t>(input["d6Pin"] | static_cast<int>(d6Pin));
        d7Pin = static_cast<uint8_t>(input["d7Pin"] | static_cast<int>(d7Pin));
        backlightPin = static_cast<uint8_t>(input["backlightPin"] | static_cast<int>(backlightPin));
        return true;
    }

    void writeJson(JsonObject output) const {
        writeDeviceBaseConfigJson(*this, output);
        output["rsPin"] = rsPin;
        output["ePin"] = ePin;
        output["d4Pin"] = d4Pin;
        output["d5Pin"] = d5Pin;
        output["d6Pin"] = d6Pin;
        output["d7Pin"] = d7Pin;
        output["backlightPin"] = backlightPin;
    }
};
#pragma pack(pop)

template <typename Config> constexpr size_t hd44780PinDisplayDeviceConfigSize(const Config&) {
    return sizeof(Config::kMagic) - 1U + sizeof(Config);
}

template <typename Config> bool decodeHd44780PinDisplayDeviceConfig(const uint8_t* blob, size_t size, Config& config) {
    return decodeValidatedFixedConfigBlob(Config::kMagic, blob, size, config);
}

} // namespace ewfm
