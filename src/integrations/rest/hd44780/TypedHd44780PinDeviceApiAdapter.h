#pragma once

#include "devices/core/ConfigCodec.h"
#include "devices/core/DeviceBaseConfig.h"
#include "devices/display/DisplayLayoutCodec.h"
#include "devices/display/DisplayLayoutProfile.h"
#include "devices/display/DisplayLayoutValidator.h"
#include "devices/display/DisplayTextPlaceholderAst.h"
#include "integrations/rest/display/DisplayDeviceApiAdapter.h"

#include <array>

namespace ewfm {

// Shared REST adapter for direct-GPIO HD44780 displays (lcd1602_pin, lcd2004_pin, ...): no
// hardware dependency at all, only optional layout-derived MetricSource links. Generalizes
// Tm1637DeviceApiAdapter's shape (another display that owns its pins outright) into a CRTP
// template, the way TypedDisplayDeviceApiAdapter generalizes the I2C-bus display adapters.
//
// Error messages are generic ("display layout is invalid", ...), matching Tm1637DeviceApiAdapter's
// own constants -- not type-specific, so they live here once instead of every concrete adapter
// declaring its own copy.
template <typename Derived, typename Device, typename Config> class TypedHd44780PinDeviceApiAdapter : public DisplayDeviceApiAdapter {
public:
    static constexpr const char* kInvalidLayoutError = "display layout is invalid";
    static constexpr const char* kLayoutSizeError = "display layout exceeds supported size";
    static constexpr const char* kLayoutDependencyCountError = "display layout exceeds supported dependency count";
    static constexpr const char* kLayoutPlaceholderError = "display layout placeholder is invalid";
    static constexpr const char* kMetricDependencyError = "display metric dependency is invalid";

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
        if (!config.parseJson(configInput, error) || !config.validate().ok()) {
            if (error == nullptr) {
                error = "device config is invalid";
            }
            return false;
        }
        if (!assignDeviceBaseConfig(request.baseConfig, config.name, config.enabled != 0U)) {
            error = "device base config is invalid";
            return false;
        }
        if (!parseDeps(configInput, request.deps, request.depCount, error, false)) {
            return false;
        }
        return encodeConfig(config, request.configBlob, error);
    }

    bool parseCreatePersistedStateRequest(const JsonObjectConst& input, DeviceCreateRequest& request,
                                          DeviceCreatePersistenceRequest& persistedRequest, const char*& error) const final {
        persistedRequest = {};
        if (!encodeLayoutRequest(input, 0U, persistedRequest.persistedStateBlob, error, kInvalidLayoutError, kLayoutSizeError)) {
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
            error = "device config is required";
            return false;
        }

        Config config = static_cast<const Device&>(runtime).config();
        if (!config.parseJson(configInput, error) || !config.validate().ok()) {
            if (error == nullptr) {
                error = "device config is invalid";
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
        if (explicitDepsProvided && !parseDeps(input, request.deps, request.depCount, error, false)) {
            return false;
        }
        if (!parseAndEncodeLayoutRequest(input, runtime.deviceId(), request.persistedStateBlob, request.deps, request.depCount, error,
                                         kInvalidLayoutError, kLayoutSizeError, kLayoutDependencyCountError)) {
            return false;
        }
        request.depsProvided = explicitDepsProvided || request.depCount > 0U;
        request.persistedStateProvided = !request.persistedStateBlob.empty();
        return true;
    }

    DeviceValidationResult validateCreateRequest(const DeviceCreateRequest& request, const DeviceRegistry& registry) const final {
        Config config{};
        if (!Derived::decodeConfig(request.configBlob.data(), request.configBlob.size(), config)) {
            return {DeviceError::InvalidConfig, "device config is invalid"};
        }
        const DeviceValidationResult depResult = validateDeps(registry, request.deps.data(), request.depCount, nullptr);
        return depResult.ok() ? config.validate() : depResult;
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
        if (!Derived::decodeConfig(request.configBlob.data(), request.configBlob.size(), config)) {
            return {DeviceError::InvalidConfig, "device config is invalid"};
        }
        const DeviceDependencyLink* deps = request.depsProvided ? request.deps.data() : runtime.dependencyLinks();
        const uint8_t depCount = request.depsProvided ? request.depCount : runtime.dependencyCount();
        const DeviceValidationResult depResult = validateDeps(registry, deps, depCount, &runtime);
        return depResult.ok() ? validatePersistedLayout(request.persistedStateBlob, request.persistedStateProvided, registry) : depResult;
    }

    DeviceValidationResult validateSetDepsRequest(const IDeviceRuntime& runtime,
                                                  const std::array<DeviceDependencyLink, kMaxDeviceDependencies>& deps,
                                                  const uint8_t depCount, const DeviceRegistry& registry) const final {
        return validateDeps(registry, deps.data(), depCount, &runtime);
    }

    void writeDeviceJson(const IDeviceRuntime& runtime, const DeviceStatus effectiveStatus, JsonObject output) const final {
        writeCommonDeviceJson(runtime, effectiveStatus, typeName(), output);
        const Device& device = static_cast<const Device&>(runtime);
        device.config().writeJson(output["config"].as<JsonObject>());
        writeDisplayLayoutProfileJson(device.displayProfile(), output["runtime"].as<JsonObject>().createNestedObject("displayProfile"));
    }

    void writeConfigJson(const IDeviceRuntime& runtime, JsonObject config) const final {
        static_cast<const Device&>(runtime).config().writeJson(config);
    }

private:
    static bool parseDeps(const JsonObjectConst& input, std::array<DeviceDependencyLink, kMaxDeviceDependencies>& deps, uint8_t& depCount,
                          const char*& error, const bool required) {
        if (!IDeviceApiAdapter::parseDependenciesJson(input, deps, depCount, error, required)) {
            return false;
        }
        for (uint8_t index = 0U; index < depCount; ++index) {
            if (deps[index].role != DeviceRole::MetricSource || deps[index].deviceId == 0U) {
                error = kMetricDependencyError;
                return false;
            }
        }
        return true;
    }

    static DeviceValidationResult validateDeps(const DeviceRegistry& registry, const DeviceDependencyLink* deps, const uint8_t depCount,
                                               const IDeviceRuntime* ignoreRuntime) {
        (void)registry;
        (void)ignoreRuntime;
        if (depCount == 0U) {
            return {};
        }
        if (deps == nullptr) {
            return {DeviceError::InvalidRelationship, kMetricDependencyError};
        }
        for (uint8_t index = 0U; index < depCount; ++index) {
            if (deps[index].role != DeviceRole::MetricSource || deps[index].deviceId == 0U) {
                return {DeviceError::InvalidRelationship, kMetricDependencyError};
            }
        }
        return {};
    }

    template <typename Blob> static bool encodeConfig(const Config& config, Blob& blob, const char*& error) {
        uint8_t buffer[kMaxDeviceConfigBytes]{};
        const size_t size = fixedConfigBlobSize(Config::kMagic, config);
        if (!encodeFixedConfigBlob(Config::kMagic, config, buffer, size) || !blob.assign(buffer, size)) {
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
            error = kInvalidLayoutError;
            return false;
        }
        return collectLayoutMetricSourceDependencies(layout, dependencies, dependencyCount, error, kInvalidLayoutError,
                                                     kLayoutDependencyCountError);
    }

    static DeviceValidationResult validatePersistedLayout(const BoundedBlob<kMaxDisplayLayoutBytes>& blob, const bool provided,
                                                          const DeviceRegistry& registry) {
        if (!provided) {
            return {};
        }
        DisplayLayoutRecordV1 layout{};
        if (!decodeDisplayLayoutBinary(blob.data(), blob.size(), layout) || !validateDisplayLayout(layout, Derived::layoutProfile()).ok()) {
            return {DeviceError::InvalidConfig, kInvalidLayoutError};
        }
        if (!validateLayoutMetricPlaceholders(layout, registry)) {
            return {DeviceError::InvalidRelationship, kLayoutPlaceholderError};
        }
        const char* imageKeyError = nullptr;
        if (!validateLayoutImageKeys(layout, imageKeyError)) {
            return {DeviceError::InvalidConfig, imageKeyError};
        }
        return {};
    }
};

} // namespace ewfm
