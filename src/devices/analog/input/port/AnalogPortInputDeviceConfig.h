#pragma once

#include "devices/analog/adc/IAdcInputDriver.h"
#include "devices/analog/input/AnalogInputPollConfig.h"
#include "devices/core/DeviceBaseConfig.h"

#include <ArduinoJson.h>
#include <cstddef>
#include <cstdint>

namespace ewfm {

constexpr DeviceTypeId kAnalogPortInputTypeId = 24;
constexpr uint32_t kAnalogPortInputConfigVersion = 1;
constexpr uint8_t kAnalogPortInputMinAdcSamples = 1;
constexpr uint8_t kAnalogPortInputMaxAdcSamples = 64;
constexpr uint8_t kAnalogPortInputDefaultAdcSamples = 8;

#pragma pack(push, 1)
struct AnalogPortInputDeviceConfigV1 : DeviceBaseConfigV1 {
    static constexpr char kMagic[] = "ANLG-PORT-1";

    AnalogPortInputDeviceConfigV1() {
        poll.adcSamples = kAnalogPortInputDefaultAdcSamples;
    }

    uint8_t gpioPin{34};
    uint8_t attenuation{static_cast<uint8_t>(AdcAttenuation::Db11)};
    AnalogInputPollConfigV1 poll{};

    bool parseJson(const JsonObjectConst& input, const char*& error);
    DeviceValidationResult validate() const;
    void writeJson(JsonObject output) const;
};
#pragma pack(pop)

constexpr size_t analogPortInputDeviceConfigSize(const AnalogPortInputDeviceConfigV1&) {
    return sizeof(AnalogPortInputDeviceConfigV1::kMagic) - 1U + sizeof(AnalogPortInputDeviceConfigV1);
}

bool analogPortInputGpioPinIsValid(uint8_t pin);

} // namespace ewfm
