#pragma once

#include "devices/core/DeviceTypes.h"

#include <ArduinoJson.h>
#include <cstdint>

namespace ewfm {

constexpr uint32_t kAnalogInputPollConfigDefaultPollMs = 1000;
constexpr uint32_t kAnalogInputPollConfigMinPollMs = 100;
constexpr uint32_t kAnalogInputPollConfigMaxPollMs = 86400000UL;
constexpr uint16_t kAnalogInputPollConfigDefaultReportDeltaMilliVolts = 10;

// adcSamples/reportAlways/reportDeltaMilliVolts/pollMs shape embedded by every AnalogInput leaf
// config (AnalogPortInputDeviceConfigV1, AnalogInputChannelDeviceConfigV1) -- composed the same
// way SensorFilterConfigV1 is embedded by every temperature sensor config.
// Only the adcSamples bounds are owner-specific (a real I2C conversion per sample on ADS1115 vs a
// cheap ESP32 ADC read for the port/CD74HC4067 backends), so those are passed into validate();
// the owner sets its own default adcSamples in its own constructor for the same reason. pollMs
// bounds and the reportDeltaMilliVolts default are identical across every backend today, so they
// live here once.
#pragma pack(push, 1)
struct AnalogInputPollConfigV1 {
    uint8_t adcSamples{1};
    uint8_t reportAlways{0};
    uint16_t reportDeltaMilliVolts{kAnalogInputPollConfigDefaultReportDeltaMilliVolts};
    uint32_t pollMs{kAnalogInputPollConfigDefaultPollMs};

    bool parseJson(const JsonObjectConst& input, const char*& error);
    DeviceValidationResult validate(uint8_t minAdcSamples, uint8_t maxAdcSamples) const;
    void writeJson(JsonObject output) const;
};
#pragma pack(pop)

} // namespace ewfm
