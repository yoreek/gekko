#pragma once

#include "devices/output/OutputDeviceConfig.h"

#include <ArduinoJson.h>
#include <cstdint>

namespace ewfm {

constexpr uint16_t percentToAnalogOutputState(uint16_t percent) {
    return static_cast<uint16_t>((static_cast<uint32_t>(percent) * static_cast<uint32_t>(kAnalogOutputLevelMax) + 50U) / 100U);
}

constexpr uint8_t analogOutputStateToPercent(uint16_t state) {
    return static_cast<uint8_t>((static_cast<uint32_t>(state) * 100U + (kAnalogOutputLevelMax / 2U)) / kAnalogOutputLevelMax);
}

template <> struct OutputDeviceValueCodec<uint16_t> {
    static bool parseJson(const JsonVariantConst& input, uint16_t& state, const char*& error);
    static bool valid(uint16_t state);
    static void writeJson(JsonObject output, const char* key, uint16_t state);
};

#pragma pack(push, 1)
struct AnalogOutputDeviceConfigV1 : OutputDeviceConfigV1<uint16_t> {};
#pragma pack(pop)

} // namespace ewfm
