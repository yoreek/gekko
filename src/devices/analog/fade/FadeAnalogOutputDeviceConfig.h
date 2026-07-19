#pragma once

#include "devices/core/DeviceBaseConfig.h"

#include <cstddef>
#include <cstdint>

namespace ewfm {

constexpr DeviceTypeId kFadeAnalogOutputDeviceTypeId = 21;
constexpr uint32_t kFadeAnalogOutputDeviceConfigVersion = 1;
constexpr uint16_t kDefaultFadeAnalogOutputMaxStep = 41U;
constexpr uint32_t kDefaultFadeAnalogOutputStepIntervalMs = 200U;

#pragma pack(push, 1)
struct FadeAnalogOutputDeviceConfigV1 : DeviceBaseConfigV1 {
    static constexpr char kMagic[] = "AFADE-1";
    uint16_t maxStep{kDefaultFadeAnalogOutputMaxStep};
    uint32_t stepIntervalMs{kDefaultFadeAnalogOutputStepIntervalMs};

    bool parseJson(const JsonObjectConst& input, const char*& error);
    DeviceValidationResult validate() const;
    void writeJson(JsonObject output) const;
};
#pragma pack(pop)

constexpr size_t fadeAnalogOutputDeviceConfigSize(const FadeAnalogOutputDeviceConfigV1&) {
    return sizeof(FadeAnalogOutputDeviceConfigV1::kMagic) - 1U + sizeof(FadeAnalogOutputDeviceConfigV1);
}

bool decodeFadeAnalogOutputDeviceConfig(const uint8_t* blob, size_t size, FadeAnalogOutputDeviceConfigV1& config);

} // namespace ewfm
