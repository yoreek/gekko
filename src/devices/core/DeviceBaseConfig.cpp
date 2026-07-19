#include "devices/core/DeviceBaseConfig.h"

#include "devices/core/ConfigCodec.h"

#include <cstring>
#include <type_traits>

namespace ewfm {

static_assert(std::is_trivially_copyable<DeviceBaseConfigV1>::value, "DeviceBaseConfigV1 must be POD");
static_assert(sizeof(DeviceBaseConfigV1) == 34, "DeviceBaseConfigV1 layout changed");
static_assert(sizeof(DeviceBaseConfigV1::kMagic) - 1U + sizeof(DeviceBaseConfigV1) <= kMaxDeviceConfigBytes,
              "DeviceBaseConfigV1 exceeds device config bound");

namespace {
size_t boundedNameLength(const char* value, size_t maxLen) {
    if (value == nullptr) {
        return 0U;
    }
    size_t length = 0U;
    while (length < maxLen && value[length] != '\0') {
        ++length;
    }
    return length;
}
} // namespace

bool readDeviceBaseConfig(const DeviceConfigBlob& blob, DeviceBaseConfigV1& config) {
    return decodeValidatedFixedConfigBlob(DeviceBaseConfigV1::kMagic, blob.data(), blob.size(), config);
}

bool writeDeviceBaseConfig(DeviceConfigBlob& blob, const DeviceBaseConfigV1& config) {
    const size_t prefixSize = deviceBaseConfigSize(config);
    if (blob.size() < prefixSize) {
        return false;
    }
    if (!encodeFixedConfigBlob(DeviceBaseConfigV1::kMagic, config, blob.data(), blob.size())) {
        return false;
    }
    return true;
}

bool parseDeviceBaseConfigJson(const JsonObjectConst& input, DeviceBaseConfigV1& config, const char*& error) {
    return config.parseJson(input, error);
}

void writeDeviceBaseConfigJson(const DeviceBaseConfigV1& config, JsonObject output) {
    config.writeJson(output);
}

DeviceValidationResult DeviceBaseConfigV1::validate() const {
    if (enabled > 1U) {
        return {DeviceError::InvalidConfig, "device enabled flag is invalid"};
    }
    const size_t nameLength = boundedNameLength(name, kMaxDeviceBaseNameLength + 1U);
    if (nameLength == 0U) {
        return {DeviceError::InvalidConfig, "device name is required"};
    }
    if (nameLength > kMaxDeviceBaseNameLength) {
        return {DeviceError::BoundsExceeded, "device name exceeds supported length"};
    }
    return {};
}

bool DeviceBaseConfigV1::parseJson(const JsonObjectConst& input, const char*& error) {
    enabled = (input["enabled"] | (enabled != 0U)) ? 1U : 0U;
    char previousName[kMaxDeviceBaseNameLength + 1]{};
    std::memcpy(previousName, name, sizeof(previousName));
    const char* newName = input["name"] | static_cast<const char*>(previousName);
    const size_t nameLength = boundedNameLength(newName, kMaxDeviceBaseNameLength + 1U);
    if (nameLength > kMaxDeviceBaseNameLength) {
        error = "device name exceeds supported length";
        return false;
    }
    std::memset(name, 0, sizeof(name));
    if (nameLength != 0U) {
        std::memcpy(name, newName, nameLength);
    }
    name[nameLength] = '\0';
    return true;
}

void DeviceBaseConfigV1::writeJson(JsonObject output) const {
    output["enabled"] = enabled != 0U;
    output["name"] = JsonString(name, JsonString::Copied);
}

} // namespace ewfm
