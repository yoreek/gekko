#include "integrations/rest/rtc_ds1302/Ds1302RtcDeviceApiAdapter.h"

#include "integrations/rest/common/RtcDeviceApiSupport.h"

namespace ewfm {
namespace {
bool parseNoDeps(const JsonObjectConst& input, const char*& error) {
    if (input.containsKey("deps")) {
        const JsonArrayConst deps = input["deps"].as<JsonArrayConst>();
        if (deps.isNull() || deps.size() > 0U) {
            error = "ds1302 does not use dependencies";
            return false;
        }
    }
    error = nullptr;
    return true;
}
} // namespace

bool Ds1302RtcDeviceApiAdapter::parseCreateExtras(const JsonObjectConst& input, const JsonObjectConst& configInput,
                                                  Ds1302RtcDeviceConfigV1& config, DeviceCreateRequest& request, const char*& error) const {
    (void)input;
    (void)config;
    request.depCount = 0U;
    request.deps = {};
    // Create requests carry deps inside "config" (matching the rtc_ds1302 config schema), same as
    // Ds3231RtcDeviceApiAdapter::parseCreateExtras - not the top-level request body.
    return parseNoDeps(configInput, error);
}

bool Ds1302RtcDeviceApiAdapter::parseUpdateExtras(const JsonObjectConst& input, const JsonObjectConst& configInput,
                                                  Ds1302RtcDeviceConfigV1& config, DeviceConfigUpdateRequest& request,
                                                  const char*& error) const {
    (void)configInput;
    (void)config;
    request.depsProvided = false;
    request.depCount = 0U;
    request.deps = {};
    return parseNoDeps(input, error);
}

DeviceValidationResult Ds1302RtcDeviceApiAdapter::validateCreateRequest(const DeviceCreateRequest& request,
                                                                        const DeviceRegistry& registry) const {
    Ds1302RtcDeviceConfigV1 config{};
    if (!decodeDs1302RtcDeviceConfig(reinterpret_cast<const uint8_t*>(request.configBlob.data()), request.configBlob.size(), config)) {
        return {DeviceError::InvalidConfig, kInvalidConfigError};
    }
    return validateAtMostOneActiveRtcSync(registry, nullptr, config.useForSystemTimeSync != 0U);
}

DeviceValidationResult Ds1302RtcDeviceApiAdapter::validateUpdateConfigRequest(const IDeviceRuntime& runtime,
                                                                              const DeviceConfigUpdateRequest& request,
                                                                              const DeviceRegistry& registry) const {
    Ds1302RtcDeviceConfigV1 config{};
    if (!decodeDs1302RtcDeviceConfig(reinterpret_cast<const uint8_t*>(request.configBlob.data()), request.configBlob.size(), config)) {
        return {DeviceError::InvalidConfig, kInvalidConfigError};
    }
    return validateAtMostOneActiveRtcSync(registry, &runtime, config.useForSystemTimeSync != 0U);
}

DeviceValidationResult
Ds1302RtcDeviceApiAdapter::validateSetDepsRequest(const IDeviceRuntime& runtime,
                                                  const std::array<DeviceDependencyLink, kMaxDeviceDependencies>& deps, uint8_t depCount,
                                                  const DeviceRegistry& registry) const {
    (void)runtime;
    (void)registry;
    if (depCount != 0U) {
        (void)deps;
        return {DeviceError::InvalidRelationship, "ds1302 does not use dependencies"};
    }
    return {};
}

void Ds1302RtcDeviceApiAdapter::writeRuntimeJson(const Ds1302RtcDevice& device, JsonObject runtimeJson) const {
    uint32_t epoch = 0;
    bool lostPower = false;
    const bool hasReading = device.latestTimeReading(epoch, lostPower);
    runtimeJson["currentEpochUtc"] = hasReading ? epoch : 0U;
    runtimeJson["lastReadOk"] = device.lastReadOk();
}

} // namespace ewfm
