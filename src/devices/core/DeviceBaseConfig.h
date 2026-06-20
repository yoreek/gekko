#pragma once

#include "devices/core/DeviceTypes.h"

#include <ArduinoJson.h>
#include <cstddef>
#include <cstdint>

namespace ewfm {

template <size_t kCapacity> bool copyBoundedText(char (&dest)[kCapacity], std::string_view value) {
    if (value.size() >= kCapacity) {
        return false;
    }
    std::memset(dest, 0, kCapacity);
    if (!value.empty()) {
        std::memcpy(dest, value.data(), value.size());
    }
    dest[value.size()] = '\0';
    return true;
}

constexpr size_t deviceBaseConfigSize(const DeviceBaseConfigV1&) {
    return sizeof(DeviceBaseConfigV1::kMagic) - 1U + sizeof(DeviceBaseConfigV1);
}

bool encodeDeviceBaseConfig(const DeviceBaseConfigV1& config, uint8_t* blob, size_t capacity);
bool decodeDeviceBaseConfig(const uint8_t* blob, size_t size, DeviceBaseConfigV1& config);
bool readDeviceBaseConfig(const DeviceConfigBlob& blob, DeviceBaseConfigV1& config);
bool writeDeviceBaseConfig(DeviceConfigBlob& blob, const DeviceBaseConfigV1& config);
DeviceValidationResult validateDeviceBaseConfig(const DeviceBaseConfigV1& config);
bool parseDeviceBaseConfigJson(const JsonObjectConst& input, DeviceBaseConfigV1& config, const char*& error);
void writeDeviceBaseConfigJson(const DeviceBaseConfigV1& config, JsonObject output);

} // namespace ewfm
