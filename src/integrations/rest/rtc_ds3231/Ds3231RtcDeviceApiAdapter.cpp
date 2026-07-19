#include "integrations/rest/rtc_ds3231/Ds3231RtcDeviceApiAdapter.h"

#include "integrations/rest/common/I2cDeviceApiSupport.h"

namespace ewfm {
namespace {
bool parseDepsField(const JsonObjectConst& input, std::array<DeviceDependencyLink, kMaxDeviceDependencies>& deps, uint8_t& depCount,
                    const char*& error) {
    return IDeviceApiAdapter::parseDependenciesJson(input, deps, depCount, error);
}

// Only one RTC device may act as the system time synchronizer at a time - RtcSyncCoordinator
// picks up whichever one is Ready and has this flag set, so allowing two would make that choice
// ambiguous. Enforced here rather than at the registry/dependency layer since it is a business
// rule about config content, not a structural relationship.
DeviceValidationResult validateAtMostOneActiveSync(const DeviceRegistry& registry, const IDeviceRuntime* self, bool requestedActive) {
    if (!requestedActive) {
        return {};
    }
    DeviceValidationResult result{};
    registry.forEachRuntime([&](const IDeviceRuntime& runtime) {
        if (!result.ok() || &runtime == self) {
            return;
        }
        const IRealTimeClockRuntime* rtc = runtime.realTimeClockRuntime();
        if (rtc != nullptr && rtc->useForSystemTimeSync()) {
            result = {DeviceError::InvalidConfig, "another RTC device is already set to sync system time"};
        }
    });
    return result;
}
} // namespace

bool Ds3231RtcDeviceApiAdapter::parseCreateExtras(const JsonObjectConst& input, const JsonObjectConst& configInput,
                                                  Ds3231RtcDeviceConfigV2& config, DeviceCreateRequest& request, const char*& error) const {
    (void)input;
    (void)config;
    return parseDepsField(configInput, request.deps, request.depCount, error);
}

bool Ds3231RtcDeviceApiAdapter::parseUpdateExtras(const JsonObjectConst& input, const JsonObjectConst& configInput,
                                                  Ds3231RtcDeviceConfigV2& config, DeviceConfigUpdateRequest& request,
                                                  const char*& error) const {
    (void)configInput;
    (void)config;
    request.depsProvided = !input["deps"].isNull();
    if (request.depsProvided && !parseDepsField(input, request.deps, request.depCount, error)) {
        return false;
    }
    return true;
}

DeviceValidationResult Ds3231RtcDeviceApiAdapter::validateCreateRequest(const DeviceCreateRequest& request,
                                                                        const DeviceRegistry& registry) const {
    Ds3231RtcDeviceConfigV2 config{};
    if (!decodeDs3231RtcDeviceConfig(reinterpret_cast<const uint8_t*>(request.configBlob.data()), request.configBlob.size(), config)) {
        return {DeviceError::InvalidConfig, kInvalidConfigError};
    }

    const DeviceValidationResult busResult =
        validateI2cBusDependency(registry, i2cBusDependencyId(request.deps.data(), request.depCount), config.i2cAddress, nullptr);
    if (!busResult.ok()) {
        return busResult;
    }
    return validateAtMostOneActiveSync(registry, nullptr, config.useForSystemTimeSync != 0U);
}

DeviceValidationResult Ds3231RtcDeviceApiAdapter::validateUpdateConfigRequest(const IDeviceRuntime& runtime,
                                                                              const DeviceConfigUpdateRequest& request,
                                                                              const DeviceRegistry& registry) const {
    Ds3231RtcDeviceConfigV2 config{};
    if (!decodeDs3231RtcDeviceConfig(reinterpret_cast<const uint8_t*>(request.configBlob.data()), request.configBlob.size(), config)) {
        return {DeviceError::InvalidConfig, kInvalidConfigError};
    }

    const DeviceId dependencyDeviceId =
        request.depsProvided ? i2cBusDependencyId(request.deps.data(), request.depCount) : runtime.dependencyDeviceId(DeviceRole::I2CBus);
    const DeviceValidationResult busResult = validateI2cBusDependency(registry, dependencyDeviceId, config.i2cAddress, &runtime);
    if (!busResult.ok()) {
        return busResult;
    }
    return validateAtMostOneActiveSync(registry, &runtime, config.useForSystemTimeSync != 0U);
}

DeviceValidationResult
Ds3231RtcDeviceApiAdapter::validateSetDepsRequest(const IDeviceRuntime& runtime,
                                                  const std::array<DeviceDependencyLink, kMaxDeviceDependencies>& deps, uint8_t depCount,
                                                  const DeviceRegistry& registry) const {
    uint8_t address = 0;
    if (!runtime.i2cAddress(address)) {
        return {DeviceError::InvalidConfig, kInvalidConfigError};
    }
    return validateI2cBusDependency(registry, i2cBusDependencyId(deps.data(), depCount), address, &runtime);
}

void Ds3231RtcDeviceApiAdapter::writeRuntimeJson(const Ds3231RtcDevice& device, JsonObject runtimeJson) const {
    uint32_t epoch = 0;
    bool lostPower = false;
    const bool hasReading = device.latestTimeReading(epoch, lostPower);
    runtimeJson["currentEpochUtc"] = hasReading ? epoch : 0U;
    runtimeJson["lastReadOk"] = device.lastReadOk();
    runtimeJson["oscillatorStopped"] = hasReading && lostPower;
}

} // namespace ewfm
