#pragma once

#include "devices/core/ConfigCodec.h"
#include "devices/core/DeviceBaseConfig.h"
#include "devices/display/DisplayLayoutCodec.h"
#include "devices/display/DisplayLayoutProfile.h"
#include "integrations/rest/display/DisplayDeviceApiAdapter.h"

namespace ewfm {

template <typename Derived, typename Device, typename Config> class TypedDisplayDeviceApiAdapter : public DisplayDeviceApiAdapter {
public:
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
            error = "device config is required";
            return false;
        }

        Config config{};
        if (!config.parseJson(configInput, error)) {
            return false;
        }
        if (!assignDeviceBaseConfig(request.baseConfig, config.name, config.enabled != 0U)) {
            error = "device base config is invalid";
            return false;
        }
        if (!IDeviceApiAdapter::parseDependenciesJson(configInput, request.deps, request.depCount, error, false)) {
            return false;
        }
        DeviceId busDeviceId = dependencyIdForRole(request.deps.data(), request.depCount, Derived::busRole());
        if (busDeviceId == 0U) {
            busDeviceId = Derived::configBusDeviceId(config);
            if (request.depCount == 0U && busDeviceId != 0U) {
                request.deps[0] = DeviceDependencyLink{Derived::busRole(), busDeviceId};
                request.depCount = 1U;
            }
        }
        if (busDeviceId == 0U) {
            error = Derived::kBusDependencyError;
            return false;
        }
        Derived::setConfigBusDeviceId(config, busDeviceId);
        if (!config.validate().ok()) {
            error = "device config is invalid";
            return false;
        }
        return encodeConfig(config, request.configBlob, error);
    }

    bool parseCreatePersistedStateRequest(const JsonObjectConst& input, DeviceCreateRequest& request,
                                          DeviceCreatePersistenceRequest& persistedRequest, const char*& error) const final {
        persistedRequest = {};
        if (!encodeLayoutRequest(input, 0, persistedRequest.persistedStateBlob, error, Derived::kInvalidLayoutError,
                                 Derived::kLayoutSizeError)) {
            return false;
        }
        if (!collectLayoutDependencies(input, request.deps, request.depCount, error)) {
            return false;
        }
        persistedRequest.persistedStateProvided = !persistedRequest.persistedStateBlob.empty();
        return true;
    }

    DeviceValidationResult validateCreateRequest(const DeviceCreateRequest& request, const DeviceRegistry& registry) const final {
        const DeviceValidationResult dependencyResult = validateDependencyShape(request.dependencyLinks(), request.dependencyCount());
        if (!dependencyResult.ok()) {
            return dependencyResult;
        }
        Config config{};
        if (!Derived::decodeConfig(request.configBlob.data(), request.configBlob.size(), config)) {
            return {DeviceError::InvalidConfig, "device config is invalid"};
        }
        return Derived::validateBusDependency(registry, request.dependencyLinks()[0].deviceId, config, nullptr);
    }

    DeviceValidationResult validateCreateRequest(const DeviceCreateRequest& request, const DeviceCreatePersistenceRequest& persistedRequest,
                                                 const DeviceRegistry& registry) const final {
        const DeviceValidationResult baseResult = validateCreateRequest(request, registry);
        return baseResult.ok()
                   ? validatePersistedLayout(persistedRequest.persistedStateBlob, persistedRequest.persistedStateProvided, registry)
                   : baseResult;
    }

    bool parseUpdateConfigRequest(const JsonObjectConst& input, IDeviceRuntime& runtime, DeviceConfigUpdateRequest& request,
                                  const char*& error) const final {
        const JsonObjectConst configInput = input["config"].as<JsonObjectConst>();
        if (configInput.isNull()) {
            error = "device config is required";
            return false;
        }

        Config config = static_cast<const Device&>(runtime).config();
        if (!config.parseJson(configInput, error)) {
            return false;
        }
        request = {};
        request.configVersion = currentConfigVersion();
        const bool explicitDepsProvided = !input["deps"].isNull();
        if (explicitDepsProvided) {
            if (!IDeviceApiAdapter::parseDependenciesJson(input, request.deps, request.depCount, error)) {
                return false;
            }
        } else {
            request.deps[0] = DeviceDependencyLink{Derived::busRole(), runtime.dependencyDeviceId(Derived::busRole())};
            request.depCount = 1U;
        }
        Derived::setConfigBusDeviceId(config, dependencyIdForRole(request.deps.data(), request.depCount, Derived::busRole()));
        if (!config.validate().ok()) {
            error = "device config is invalid";
            return false;
        }
        if (!assignDeviceBaseConfig(request.baseConfig, config.name, config.enabled != 0U)) {
            error = "device base config is invalid";
            return false;
        }
        if (!encodeConfig(config, request.configBlob, error)) {
            return false;
        }
        if (!collectLayoutDependencies(input, request.deps, request.depCount, error)) {
            return false;
        }
        request.depsProvided = explicitDepsProvided || request.depCount > 1U;
        if (!encodeLayoutRequest(input, runtime.deviceId(), request.persistedStateBlob, error, Derived::kInvalidLayoutError,
                                 Derived::kLayoutSizeError)) {
            return false;
        }
        request.persistedStateProvided = !request.persistedStateBlob.empty();
        return true;
    }

    DeviceValidationResult validateUpdateConfigRequest(const IDeviceRuntime& runtime, const DeviceConfigUpdateRequest& request,
                                                       const DeviceRegistry& registry) const final {
        Config config{};
        if (!Derived::decodeConfig(request.configBlob.data(), request.configBlob.size(), config)) {
            return {DeviceError::InvalidConfig, "device config is invalid"};
        }

        DeviceId busDeviceId = runtime.dependencyDeviceId(Derived::busRole());
        if (request.depsProvided) {
            const DeviceValidationResult dependencyResult = validateDependencyShape(request.deps.data(), request.depCount);
            if (!dependencyResult.ok()) {
                return dependencyResult;
            }
            busDeviceId = request.deps[0].deviceId;
        }
        const DeviceValidationResult busResult = Derived::validateBusDependency(registry, busDeviceId, config, &runtime);
        return busResult.ok() ? validatePersistedLayout(request.persistedStateBlob, request.persistedStateProvided, registry) : busResult;
    }

    DeviceValidationResult validateSetDepsRequest(const IDeviceRuntime& runtime,
                                                  const std::array<DeviceDependencyLink, kMaxDeviceDependencies>& dependencies,
                                                  const uint8_t dependencyCount, const DeviceRegistry& registry) const final {
        const DeviceValidationResult dependencyResult = validateDependencyShape(dependencies.data(), dependencyCount);
        if (!dependencyResult.ok()) {
            return dependencyResult;
        }
        return Derived::validateBusDependency(registry, dependencies[0].deviceId, static_cast<const Device&>(runtime).config(), &runtime);
    }

    void writeDeviceJson(const IDeviceRuntime& runtime, const DeviceStatus effectiveStatus, JsonObject output) const final {
        writeCommonDeviceJson(runtime, effectiveStatus, typeName(), output);
        writeConfigJson(runtime, output["config"].as<JsonObject>());
        JsonObject runtimeJson = output["runtime"].as<JsonObject>();
        JsonObject profileJson = runtimeJson.createNestedObject("displayProfile");
        writeDisplayLayoutProfileJson(static_cast<const Device&>(runtime).displayProfile(), profileJson);
    }

    void writeConfigJson(const IDeviceRuntime& runtime, JsonObject config) const final {
        static_cast<const Device&>(runtime).config().writeJson(config);
    }

protected:
    static size_t configSize(const Config& config) {
        return fixedConfigBlobSize(Config::kMagic, config);
    }

    static bool encodeConfig(const Config& config, uint8_t* blob, size_t capacity) {
        return encodeFixedConfigBlob(Config::kMagic, config, blob, capacity);
    }

private:
    static DeviceId dependencyIdForRole(const DeviceDependencyLink* dependencies, const uint8_t dependencyCount, const DeviceRole role) {
        if (dependencies == nullptr) {
            return 0;
        }
        for (uint8_t index = 0; index < dependencyCount; ++index) {
            if (dependencies[index].role == role) {
                return dependencies[index].deviceId;
            }
        }
        return 0;
    }

    template <typename Blob> static bool encodeConfig(const Config& config, Blob& blob, const char*& error) {
        uint8_t buffer[kMaxDeviceConfigBytes]{};
        const size_t size = Derived::configSize(config);
        if (!Derived::encodeConfig(config, buffer, size) || !blob.assign(buffer, size)) {
            error = "failed to encode device config";
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

    static DeviceValidationResult validateDependencyShape(const DeviceDependencyLink* dependencies, const uint8_t dependencyCount) {
        if (dependencies == nullptr || dependencyCount < 1U || dependencies[0].role != Derived::busRole() ||
            dependencies[0].deviceId == 0U) {
            return {DeviceError::InvalidRelationship, Derived::kBusDependencyError};
        }
        for (uint8_t index = 1; index < dependencyCount; ++index) {
            if (dependencies[index].role != DeviceRole::MetricSource || dependencies[index].deviceId == 0U) {
                return {DeviceError::InvalidRelationship, Derived::kMetricDependencyError};
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
        if (!decodeDisplayLayoutBinary(blob.data(), blob.size(), layout)) {
            return {DeviceError::InvalidConfig, Derived::kInvalidLayoutError};
        }
        if (!validateLayoutMetricPlaceholders(layout, registry)) {
            return {DeviceError::InvalidRelationship, Derived::kLayoutPlaceholderError};
        }
        return {};
    }
};

} // namespace ewfm
