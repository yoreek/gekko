#pragma once

#include "devices/core/DeviceBaseConfig.h"

namespace ewfm {

constexpr DeviceTypeId kAnalogOutputComposerDeviceTypeId = 23;
constexpr uint32_t kAnalogOutputComposerDeviceConfigVersion = 1;

#pragma pack(push, 1)
struct AnalogOutputComposerDeviceConfigV1 : DeviceBaseConfigV1 {
    static constexpr char kMagic[] = "ACOMP-1";

    bool parseJson(const JsonObjectConst& input, const char*& error);
    DeviceValidationResult validate() const;
    void writeJson(JsonObject output) const;
};
#pragma pack(pop)

constexpr size_t analogOutputComposerDeviceConfigSize(const AnalogOutputComposerDeviceConfigV1&) {
    return sizeof(AnalogOutputComposerDeviceConfigV1::kMagic) - 1U + sizeof(AnalogOutputComposerDeviceConfigV1);
}

bool decodeAnalogOutputComposerDeviceConfig(const uint8_t* blob, size_t size, AnalogOutputComposerDeviceConfigV1& config);

} // namespace ewfm
