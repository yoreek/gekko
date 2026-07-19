#include "integrations/rest/dosing_pump/DosingPumpDeviceApiAdapter.h"

namespace ewfm {

namespace {
bool hasDuplicateDeviceId(const DeviceDependencyLink* links, uint8_t count) {
    for (uint8_t index = 0; index < count; ++index) {
        for (uint8_t otherIndex = index + 1; otherIndex < count; ++otherIndex) {
            if (links[index].deviceId == links[otherIndex].deviceId) {
                return true;
            }
        }
    }
    return false;
}

// One required switch (the pump motor) plus at most one optional condition (the container's
// low-level sensor, with a per-link invert for sensor polarity).
bool parseDepsField(const JsonObjectConst& input, std::array<DeviceDependencyLink, kMaxDeviceDependencies>& deps, uint8_t& depCount,
                    const char*& error) {
    if (!IDeviceApiAdapter::parseDependenciesJson(input, deps, depCount, error)) {
        return false;
    }
    bool hasSwitch = false;
    bool hasCondition = false;
    for (uint8_t index = 0; index < depCount; ++index) {
        const DeviceRole role = deps[index].role;
        if (role == DeviceRole::Switch) {
            if (hasSwitch) {
                error = "dosing pump supports only one switch dependency";
                return false;
            }
            hasSwitch = true;
        } else if (role == DeviceRole::Condition) {
            if (hasCondition) {
                error = "dosing pump supports only one level sensor dependency";
                return false;
            }
            hasCondition = true;
        } else {
            error = "dosing pump dependency role is invalid, use role=switch or role=condition";
            return false;
        }
    }
    if (!hasSwitch) {
        error = "dosing pump requires a switch dependency";
        return false;
    }
    if (hasDuplicateDeviceId(deps.data(), depCount)) {
        error = "dosing pump dependency device id is duplicated";
        return false;
    }
    return true;
}

DeviceValidationResult validateCapability(const DeviceRegistry& registry, DeviceRole role, DeviceId deviceId) {
    const IDeviceRuntime* dependency = registry.runtime(deviceId);
    if (dependency == nullptr) {
        return {DeviceError::InvalidRelationship, "dosing pump dependency is missing"};
    }
    if (role == DeviceRole::Condition) {
        if (dependency->statusRuntime() == nullptr) {
            return {DeviceError::InvalidRelationship, "level sensor dependency lacks status capability"};
        }
        return {};
    }
    const ISwitchOutputRuntime* switchOutput = dependency->switchOutputRuntime();
    if (switchOutput == nullptr) {
        return {DeviceError::InvalidRelationship, "switch dependency lacks output capability"};
    }
    return {};
}

DeviceValidationResult validateDepsSet(const DeviceRegistry& registry, const DeviceDependencyLink* links, uint8_t count) {
    if (hasDuplicateDeviceId(links, count)) {
        return {DeviceError::InvalidRelationship, "dosing pump dependency device id is duplicated"};
    }
    const DeviceDependencyLink* switchLink = nullptr;
    const DeviceDependencyLink* conditionLink = nullptr;
    for (uint8_t index = 0; index < count; ++index) {
        if (links[index].role == DeviceRole::Switch) {
            switchLink = &links[index];
        } else if (links[index].role == DeviceRole::Condition) {
            if (conditionLink != nullptr) {
                return {DeviceError::InvalidRelationship, "dosing pump supports only one level sensor dependency"};
            }
            conditionLink = &links[index];
        }
    }
    if (switchLink == nullptr) {
        return {DeviceError::InvalidRelationship, "dosing pump requires a switch dependency"};
    }
    const DeviceValidationResult switchResult = validateCapability(registry, DeviceRole::Switch, switchLink->deviceId);
    if (!switchResult.ok()) {
        return switchResult;
    }
    if (conditionLink != nullptr) {
        const DeviceValidationResult conditionResult = validateCapability(registry, DeviceRole::Condition, conditionLink->deviceId);
        if (!conditionResult.ok()) {
            return conditionResult;
        }
    }
    return {};
}
} // namespace

bool DosingPumpDeviceApiAdapter::parseCreateExtras(const JsonObjectConst& input, const JsonObjectConst& configInput,
                                                   DosingPumpDeviceConfigV1& config, DeviceCreateRequest& request,
                                                   const char*& error) const {
    (void)input;
    (void)config;
    return parseDepsField(configInput, request.deps, request.depCount, error);
}

bool DosingPumpDeviceApiAdapter::parseUpdateExtras(const JsonObjectConst& input, const JsonObjectConst& configInput,
                                                   DosingPumpDeviceConfigV1& config, DeviceConfigUpdateRequest& request,
                                                   const char*& error) const {
    (void)configInput;
    (void)config;
    request.depsProvided = !input["deps"].isNull();
    if (request.depsProvided && !parseDepsField(input, request.deps, request.depCount, error)) {
        return false;
    }
    return true;
}

DeviceValidationResult DosingPumpDeviceApiAdapter::validateCreateRequest(const DeviceCreateRequest& request,
                                                                         const DeviceRegistry& registry) const {
    DosingPumpDeviceConfigV1 config{};
    if (!decodeValidatedFixedConfigBlob(DosingPumpDeviceConfigV1::kMagic, reinterpret_cast<const uint8_t*>(request.configBlob.data()),
                                        request.configBlob.size(), config)) {
        return {DeviceError::InvalidConfig, kInvalidConfigError};
    }
    const DeviceValidationResult depsResult = validateDepsSet(registry, request.dependencyLinks(), request.dependencyCount());
    if (!depsResult.ok()) {
        return depsResult;
    }
    return config.validate();
}

DeviceValidationResult DosingPumpDeviceApiAdapter::validateUpdateConfigRequest(const IDeviceRuntime& runtime,
                                                                               const DeviceConfigUpdateRequest& request,
                                                                               const DeviceRegistry& registry) const {
    DosingPumpDeviceConfigV1 config{};
    if (!decodeValidatedFixedConfigBlob(DosingPumpDeviceConfigV1::kMagic, reinterpret_cast<const uint8_t*>(request.configBlob.data()),
                                        request.configBlob.size(), config)) {
        return {DeviceError::InvalidConfig, kInvalidConfigError};
    }
    if (request.depsProvided) {
        const DeviceValidationResult depsResult = validateDepsSet(registry, request.deps.data(), request.depCount);
        if (!depsResult.ok()) {
            return depsResult;
        }
    } else {
        const DeviceValidationResult depsResult = validateDepsSet(registry, runtime.dependencyLinks(), runtime.dependencyCount());
        if (!depsResult.ok()) {
            return depsResult;
        }
    }
    return config.validate();
}

DeviceValidationResult
DosingPumpDeviceApiAdapter::validateSetDepsRequest(const IDeviceRuntime& runtime,
                                                   const std::array<DeviceDependencyLink, kMaxDeviceDependencies>& deps, uint8_t depCount,
                                                   const DeviceRegistry& registry) const {
    (void)runtime;
    return validateDepsSet(registry, deps.data(), depCount);
}

void DosingPumpDeviceApiAdapter::writeRuntimeJson(const DosingPumpDevice& device, JsonObject runtimeJson) const {
    JsonObject outputJson = runtimeJson.createNestedObject("output");
    outputJson["state"] = device.running() ? "dosing" : "idle";
    outputJson["autoMode"] = device.autoMode();
    outputJson["timeValid"] = device.timeValid();

    if (device.running()) {
        const char* doseType = "manual";
        if (device.runType() == DosingRunType::Scheduled) {
            doseType = "schedule";
        } else if (device.runType() == DosingRunType::Calibration) {
            doseType = "calibration";
        }
        outputJson["doseType"] = doseType;
        outputJson["dosingTargetMl"] = static_cast<float>(device.runTargetCentiMl()) / 100.0F;
        outputJson["dosedMl"] = static_cast<float>(device.runDosedCentiMl()) / 100.0F;
        outputJson["dosingRemainingSec"] = (device.runRemainingMs() + 999U) / 1000U;
        outputJson["dosingTotalSec"] = (device.runTotalMs() + 999U) / 1000U;
    }

    outputJson["lastRunDosedMl"] = static_cast<float>(device.lastRunDosedCentiMl()) / 100.0F;
    outputJson["todayDosedMl"] = static_cast<float>(device.todayDosedCentiMl()) / 100.0F;
    outputJson["todayTargetMl"] = static_cast<float>(device.todayTargetCentiMlCached()) / 100.0F;

    uint32_t nextEpoch = 0U;
    uint16_t nextAmountCentiMl = 0U;
    if (device.nextDoseAt(nextEpoch, nextAmountCentiMl)) {
        outputJson["nextDoseAt"] = nextEpoch;
        outputJson["nextDoseAmountMl"] = static_cast<float>(nextAmountCentiMl) / 100.0F;
    }
    if (device.lastDoseEpoch() != 0U) {
        JsonObject lastDose = outputJson.createNestedObject("lastDose");
        lastDose["at"] = device.lastDoseEpoch();
        lastDose["type"] = device.lastDoseType() == DosingRunType::Scheduled ? "schedule" : "manual";
        lastDose["amountMl"] = static_cast<float>(device.lastDoseAmountCentiMl()) / 100.0F;
    }

    uint32_t days = 0U;
    if (device.daysLeft(days)) {
        outputJson["daysLeft"] = days;
    }

    const DosingPumpDeviceConfigV1& config = device.config();
    JsonObject container = outputJson.createNestedObject("container");
    container["capacityMl"] = config.containerCapacityMl;
    container["currentMl"] = static_cast<float>(device.containerCurrentCentiMl()) / 100.0F;
    const uint32_t capacityCentiMl = static_cast<uint32_t>(config.containerCapacityMl) * 100UL;
    const uint32_t percent = capacityCentiMl > 0U ? device.containerCurrentCentiMl() * 100UL / capacityCentiMl : 0U;
    container["percent"] = percent;
    container["empty"] = device.containerEmpty();
    container["sensorPresent"] = device.levelSensorPresent();
    const char* containerStatus = "normal";
    if (device.containerEmpty()) {
        containerStatus = "critical";
    } else if (percent <= config.thresholdPercent) {
        containerStatus = "warning";
    }
    container["status"] = containerStatus;

    JsonArray skipNext = outputJson.createNestedArray("skipNext");
    for (uint8_t index = 0U; index < config.doseCount; ++index) {
        skipNext.add((device.skipNextMask() & (1U << index)) != 0U);
    }
}

} // namespace ewfm
