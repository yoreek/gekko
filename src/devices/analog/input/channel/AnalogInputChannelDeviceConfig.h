#pragma once

#include "devices/analog/input/AnalogInputPollConfig.h"
#include "devices/core/DeviceBaseConfig.h"

#include <ArduinoJson.h>
#include <cstddef>
#include <cstdint>

namespace ewfm {

constexpr DeviceTypeId kAnalogInputChannelTypeId = 26;
constexpr uint32_t kAnalogInputChannelConfigVersion = 1;
// Any current AnalogInputHub backend tops out at 16 channels (CD74HC4067); used as a cheap,
// dependency-independent sanity bound. The real bound (the attached hub's actual channelCount())
// is enforced by the REST adapter, which has registry access to the dependency -- mirrors
// PortExpanderSwitchDeviceConfig's kMaxPortExpanderChannel for the same reason.
constexpr uint8_t kAnalogInputChannelMaxChannel = 15;
constexpr uint8_t kAnalogInputChannelMinAdcSamples = 1;
// ADS1115 costs a real I2C conversion per sample; the ESP32-ADC-backed paths (port, CD74HC4067)
// are a cheap synchronous read. 32 is generous enough to cover either without being backend-aware.
constexpr uint8_t kAnalogInputChannelMaxAdcSamples = 32;
constexpr uint8_t kAnalogInputChannelDefaultAdcSamples = 4;

// One channel of whatever AnalogInputHub is wired up -- deliberately backend-agnostic (see
// AnalogInputChannelDevice), so this is the *only* channel config, not one per hub chip.
#pragma pack(push, 1)
struct AnalogInputChannelDeviceConfigV1 : DeviceBaseConfigV1 {
    static constexpr char kMagic[] = "ANLG-CHAN-1";

    AnalogInputChannelDeviceConfigV1() {
        poll.adcSamples = kAnalogInputChannelDefaultAdcSamples;
    }

    uint8_t channel{0};
    AnalogInputPollConfigV1 poll{};

    bool parseJson(const JsonObjectConst& input, const char*& error);
    DeviceValidationResult validate() const;
    void writeJson(JsonObject output) const;
};
#pragma pack(pop)

constexpr size_t analogInputChannelDeviceConfigSize(const AnalogInputChannelDeviceConfigV1&) {
    return sizeof(AnalogInputChannelDeviceConfigV1::kMagic) - 1U + sizeof(AnalogInputChannelDeviceConfigV1);
}

} // namespace ewfm
