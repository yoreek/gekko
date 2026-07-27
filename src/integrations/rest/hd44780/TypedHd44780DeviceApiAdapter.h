#pragma once

#include "devices/core/ConfigCodec.h"
#include "devices/core/DeviceBaseConfig.h"
#include "devices/core/DeviceDependencyValidation.h"
#include "devices/display/DisplayLayoutCodec.h"
#include "devices/display/DisplayLayoutProfile.h"
#include "devices/display/DisplayLayoutValidator.h"
#include "devices/display/DisplayTextPlaceholderAst.h"
#include "devices/display/hd44780/Hd44780ChannelConfig.h"
#include "integrations/rest/display/DisplayDeviceApiAdapter.h"

namespace ewfm {

// Shared REST adapter for character displays. Hardware dependencies remain ordered Switch links;
// layout and metric-source dependencies are persisted through the common display API.
template <typename Derived, typename Device, typename Config> class TypedHd44780DeviceApiAdapter : public DisplayDeviceApiAdapter {
public:
    static constexpr const char* kConfigRequiredError = "device config is required";
    static constexpr const char* kInvalidConfigError = "device config is invalid";
    static constexpr const char* kEncodeError = "failed to encode device config";

    DeviceTypeId typeId() const final {
        return Device::descriptor().typeId;
    }

    const char* typeName() const final {
        return Derived::kTypeName;
    }

    uint32_t currentConfigVersion() const final {
        return Device::descriptor().currentConfigVersion;
    }

    bool parseCreateRequest(const JsonObjectConst& input, DeviceCreateRequest& request, const char*& error) const final {
        request = {};
        request.typeId = typeId();
        request.configVersion = currentConfigVersion();

        const JsonObjectConst configInput = input["config"].as<JsonObjectConst>();
        if (configInput.isNull()) {
            error = kConfigRequiredError;
            return false;
        }

        Config config{};
        if (!config.parseJson(configInput, error) || !config.validate().ok()) {
            if (error == nullptr) {
                error = kInvalidConfigError;
            }
            return false;
        }
        if (!assignDeviceBaseConfig(request.baseConfig, config.name, config.enabled != 0U)) {
            error = "device base config is invalid";
            return false;
        }
        return parseCreateExtras(input, configInput, config, request, error) && encodeConfig(config, request.configBlob, error);
    }

    bool parseCreatePersistedStateRequest(const JsonObjectConst& input, DeviceCreateRequest& request,
                                          DeviceCreatePersistenceRequest& persistedRequest, const char*& error) const final {
        persistedRequest = {};
        if (!encodeLayoutRequest(input, 0U, persistedRequest.persistedStateBlob, error, Derived::kInvalidLayoutError,
                                 Derived::kLayoutSizeError)) {
            return false;
        }
        if (!collectLayoutDependencies(input, request.deps, request.depCount, error)) {
            return false;
        }
        persistedRequest.persistedStateProvided = !persistedRequest.persistedStateBlob.empty();
        return true;
    }

    bool parseUpdateConfigRequest(const JsonObjectConst& input, IDeviceRuntime& runtime, DeviceConfigUpdateRequest& request,
                                  const char*& error) const final {
        const JsonObjectConst configInput = input["config"].as<JsonObjectConst>();
        if (configInput.isNull()) {
            error = kConfigRequiredError;
            return false;
        }

        Config config = static_cast<const Device&>(runtime).config();
        if (!config.parseJson(configInput, error) || !config.validate().ok()) {
            if (error == nullptr) {
                error = kInvalidConfigError;
            }
            return false;
        }

        request = {};
        request.configVersion = currentConfigVersion();
        if (!assignDeviceBaseConfig(request.baseConfig, config.name, config.enabled != 0U)) {
            error = "device base config is invalid";
            return false;
        }
        if (!encodeConfig(config, request.configBlob, error)) {
            return false;
        }

        const bool explicitDepsProvided = !input["deps"].isNull();
        if (explicitDepsProvided) {
            if (!IDeviceApiAdapter::parseDependenciesJson(input, request.deps, request.depCount, error)) {
                return false;
            }
        } else {
            const DeviceDependencyLink* currentDeps = runtime.dependencyLinks();
            request.depCount = runtime.dependencyCount();
            if (currentDeps == nullptr || request.depCount > kMaxDeviceDependencies) {
                error = Derived::kDependencyCountError;
                return false;
            }
            for (uint8_t index = 0U; index < request.depCount; ++index) {
                request.deps[index] = currentDeps[index];
            }
        }

        if (!collectLayoutDependencies(input, request.deps, request.depCount, error)) {
            return false;
        }
        request.depsProvided = explicitDepsProvided || request.depCount > runtime.dependencyCount();
        if (!encodeLayoutRequest(input, runtime.deviceId(), request.persistedStateBlob, error, Derived::kInvalidLayoutError,
                                 Derived::kLayoutSizeError)) {
            return false;
        }
        request.persistedStateProvided = !request.persistedStateBlob.empty();
        return true;
    }

    DeviceValidationResult validateCreateRequest(const DeviceCreateRequest& request, const DeviceRegistry& registry) const final {
        Config config{};
        if (!decodeConfig(request.configBlob.data(), request.configBlob.size(), config)) {
            return {DeviceError::InvalidConfig, kInvalidConfigError};
        }
        const DeviceValidationResult dependencyResult =
            validateDependencies(registry, request.deps.data(), request.depCount, nullptr, config, nullptr, 0U);
        if (!dependencyResult.ok()) {
            return dependencyResult;
        }
        return config.validate();
    }

    DeviceValidationResult validateCreateRequest(const DeviceCreateRequest& request, const DeviceCreatePersistenceRequest& persistedRequest,
                                                 const DeviceRegistry& registry) const final {
        const DeviceValidationResult baseResult = validateCreateRequest(request, registry);
        return baseResult.ok()
                   ? validatePersistedLayout(persistedRequest.persistedStateBlob, persistedRequest.persistedStateProvided, registry)
                   : baseResult;
    }

    DeviceValidationResult validateUpdateConfigRequest(const IDeviceRuntime& runtime, const DeviceConfigUpdateRequest& request,
                                                       const DeviceRegistry& registry) const final {
        Config config{};
        if (!decodeConfig(request.configBlob.data(), request.configBlob.size(), config)) {
            return {DeviceError::InvalidConfig, kInvalidConfigError};
        }
        const DeviceDependencyLink* deps = request.depsProvided ? request.deps.data() : runtime.dependencyLinks();
        const uint8_t depCount = request.depsProvided ? request.depCount : runtime.dependencyCount();
        const DeviceValidationResult dependencyResult = validateDependencies(registry, deps, depCount, &runtime, config, nullptr, 0U);
        if (!dependencyResult.ok()) {
            return dependencyResult;
        }
        return validatePersistedLayout(request.persistedStateBlob, request.persistedStateProvided, registry);
    }

    DeviceValidationResult validateSetDepsRequest(const IDeviceRuntime& runtime,
                                                  const std::array<DeviceDependencyLink, kMaxDeviceDependencies>& deps,
                                                  const uint8_t depCount, const DeviceRegistry& registry) const final {
        uint8_t slots[7U]{};
        const uint8_t slotCount = runtime.dependencySlots(slots, 7U);
        return validateDependencies(registry, deps.data(), depCount, &runtime, static_cast<const Device&>(runtime).config(), slots,
                                    slotCount);
    }

    void writeDeviceJson(const IDeviceRuntime& runtime, const DeviceStatus effectiveStatus, JsonObject output) const final {
        writeCommonDeviceJson(runtime, effectiveStatus, typeName(), output);
        const Device& device = static_cast<const Device&>(runtime);
        device.config().writeJson(output["config"].as<JsonObject>());
        JsonObject profile = output["runtime"].as<JsonObject>().createNestedObject("displayProfile");
        writeDisplayLayoutProfileJson(device.displayProfile(), profile);
    }

    void writeConfigJson(const IDeviceRuntime& runtime, JsonObject config) const final {
        static_cast<const Device&>(runtime).config().writeJson(config);
    }

protected:
    static bool decodeConfig(const uint8_t* data, size_t size, Config& config) {
        return Derived::decodeConfig(data, size, config);
    }

    static bool parseCreateExtras(const JsonObjectConst& input, const JsonObjectConst& configInput, Config& config,
                                  DeviceCreateRequest& request, const char*& error) {
        (void)input;
        (void)config;
        return IDeviceApiAdapter::parseDependenciesJson(configInput, request.deps, request.depCount, error);
    }

private:
    template <typename Blob> static bool encodeConfig(const Config& config, Blob& blob, const char*& error) {
        uint8_t buffer[kMaxDeviceConfigBytes]{};
        const size_t size = fixedConfigBlobSize(Config::kMagic, config);
        if (!encodeFixedConfigBlob(Config::kMagic, config, buffer, size) || !blob.assign(buffer, size)) {
            error = kEncodeError;
            return false;
        }
        return true;
    }

    static bool collectLayoutDependencies(const JsonObjectConst& input,
                                          std::array<DeviceDependencyLink, kMaxDeviceDependencies>& dependencies, uint8_t& dependencyCount,
                                          const char*& error) {
        const JsonObjectConst layoutInput = input["config"]["layout"].as<JsonObjectConst>();
        if (layoutInput.isNull()) {
            return true;
        }
        DisplayLayoutRecordV1 layout{};
        if (!parseDisplayLayoutJson(layoutInput, layout)) {
            error = Derived::kInvalidLayoutError;
            return false;
        }
        return collectLayoutMetricSourceDependencies(layout, dependencies, dependencyCount, error, Derived::kInvalidLayoutError,
                                                     Derived::kLayoutDependencyCountError);
    }

    static DeviceValidationResult validateDependencies(const DeviceRegistry& registry, const DeviceDependencyLink* deps,
                                                       const uint8_t depCount, const IDeviceRuntime* ignoreRuntime, const Config& config,
                                                       const uint8_t* slots, uint8_t slotCount) {
        uint8_t localSlots[7U]{};
        if (slots == nullptr) {
            slotCount = hd44780ConfigChannels(Derived::channelsOf(config), localSlots, 7U);
            slots = localSlots;
        }
        if (deps == nullptr || slotCount == 0U || slotCount > depCount) {
            return {DeviceError::InvalidRelationship, Derived::kDependencyCountError};
        }
        if (dependencyLinksHaveDuplicateDeviceIds(deps, slots, slotCount)) {
            return {DeviceError::InvalidRelationship, "switch dependency device id is duplicated"};
        }
        for (uint8_t index = 0U; index < slotCount; ++index) {
            const uint8_t slot = slots[index];
            if (slot >= depCount || deps[slot].role != DeviceRole::Switch || deps[slot].deviceId == 0U) {
                return {DeviceError::InvalidRelationship, Derived::kDepsRequiredError};
            }
            const IDeviceRuntime* dependency = registry.runtime(deps[slot].deviceId);
            if (dependency == nullptr || dependency->switchOutputRuntime() == nullptr || dependency == ignoreRuntime) {
                return {DeviceError::InvalidRelationship, "switch dependency is missing or invalid"};
            }
        }
        for (uint8_t index = slotCount; index < depCount; ++index) {
            if (deps[index].role != DeviceRole::MetricSource || deps[index].deviceId == 0U) {
                return {DeviceError::InvalidRelationship, "display metric dependency is invalid"};
            }
        }
        return {};
    }

    static DeviceValidationResult validatePersistedLayout(const BoundedBlob<kMaxDisplayLayoutBytes>& blob, const bool provided,
                                                          const DeviceRegistry& registry) {
        if (!provided) {
            return {};
        }
        DisplayLayoutRecordV1 layout{};
        if (!decodeDisplayLayoutBinary(blob.data(), blob.size(), layout) || !validateDisplayLayout(layout, Derived::layoutProfile()).ok()) {
            return {DeviceError::InvalidConfig, Derived::kInvalidLayoutError};
        }
        if (!validateLayoutMetricPlaceholders(layout, registry)) {
            return {DeviceError::InvalidRelationship, Derived::kLayoutPlaceholderError};
        }
        return {};
    }
};

} // namespace ewfm
