#include "integrations/rest/tm1637/Tm1637DeviceApiAdapter.h"

#include "devices/core/ConfigCodec.h"

namespace ewfm {
namespace {
bool configFromJson(const JsonObjectConst& input, Tm1637DeviceConfigV2& config, const char*& error) {
    if (!config.parseJson(input, error)) {
        return false;
    }
    if (!config.validate().ok()) {
        error = Tm1637DeviceApiAdapter::kInvalidConfigError;
        return false;
    }
    return true;
}

bool encodeConfigBlob(const Tm1637DeviceConfigV2& config, BoundedBlob<kMaxDeviceConfigBytes>& blob, const char*& error) {
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    const size_t size = tm1637DeviceConfigSize(config);
    if (!encodeFixedConfigBlob(Tm1637DeviceConfigV2::kMagic, config, buffer, size) || !blob.assign(buffer, size)) {
        error = "failed to encode device config";
        return false;
    }
    return true;
}
} // namespace

const Tm1637DeviceApiAdapter& Tm1637DeviceApiAdapter::instance() {
    static const Tm1637DeviceApiAdapter adapter;
    return adapter;
}

DisplayLayoutProfile Tm1637DeviceApiAdapter::layoutProfile() {
    return segmentDisplayLayoutProfile(Tm1637SegmentCodec::kDigitCount);
}

DeviceTypeId Tm1637DeviceApiAdapter::typeId() const {
    return Tm1637Device::descriptor().typeId;
}

const char* Tm1637DeviceApiAdapter::typeName() const {
    return kTypeName;
}

uint32_t Tm1637DeviceApiAdapter::currentConfigVersion() const {
    return Tm1637Device::descriptor().currentConfigVersion;
}

bool Tm1637DeviceApiAdapter::parseDeps(const JsonObjectConst& input, std::array<DeviceDependencyLink, kMaxDeviceDependencies>& deps,
                                       uint8_t& depCount, const char*& error, const bool required) {
    if (!IDeviceApiAdapter::parseDependenciesJson(input, deps, depCount, error, required)) {
        return false;
    }
    // CLK/DIO are config pins now, so every remaining dependency comes from layout placeholders.
    for (uint8_t index = 0U; index < depCount; ++index) {
        if (deps[index].role != DeviceRole::MetricSource || deps[index].deviceId == 0U) {
            error = kMetricDependencyError;
            return false;
        }
    }
    return true;
}

bool Tm1637DeviceApiAdapter::parseCreateRequest(const JsonObjectConst& input, DeviceCreateRequest& request, const char*& error) const {
    request = {};
    request.typeId = typeId();
    request.configVersion = currentConfigVersion();

    const JsonObjectConst configInput = input["config"].as<JsonObjectConst>();
    if (configInput.isNull()) {
        error = "device config is required";
        return false;
    }

    Tm1637DeviceConfigV2 config{};
    if (!configFromJson(configInput, config, error)) {
        return false;
    }
    if (!assignDeviceBaseConfig(request.baseConfig, config.name, config.enabled != 0U)) {
        error = "device base config is invalid";
        return false;
    }
    if (!parseDeps(configInput, request.deps, request.depCount, error, false)) {
        return false;
    }
    if (!encodeConfigBlob(config, request.configBlob, error)) {
        return false;
    }
    return true;
}

bool Tm1637DeviceApiAdapter::parseCreatePersistedStateRequest(const JsonObjectConst& input, DeviceCreateRequest& request,
                                                              DeviceCreatePersistenceRequest& persistedRequest, const char*& error) const {
    persistedRequest = {};
    if (!encodeLayoutRequest(input, 0U, persistedRequest.persistedStateBlob, error, kInvalidLayoutError, kLayoutSizeError)) {
        return false;
    }
    const JsonObjectConst layoutInput = input["config"]["layout"].as<JsonObjectConst>();
    if (!layoutInput.isNull()) {
        DisplayLayoutRecordV1 layout{};
        if (!parseDisplayLayoutJson(layoutInput, layout) ||
            !collectLayoutMetricSourceDependencies(layout, request.deps, request.depCount, error, kInvalidLayoutError,
                                                   kLayoutDependencyCountError)) {
            return false;
        }
    }
    persistedRequest.persistedStateProvided = !persistedRequest.persistedStateBlob.empty();
    return true;
}

bool Tm1637DeviceApiAdapter::parseUpdateConfigRequest(const JsonObjectConst& input, IDeviceRuntime& runtime,
                                                      DeviceConfigUpdateRequest& request, const char*& error) const {
    const JsonObjectConst configInput = input["config"].as<JsonObjectConst>();
    if (configInput.isNull()) {
        error = "device config is required";
        return false;
    }

    Tm1637DeviceConfigV2 config = device(runtime).config();
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
    if (!encodeConfigBlob(config, request.configBlob, error)) {
        return false;
    }

    const bool explicitDepsProvided = !input["deps"].isNull();
    if (explicitDepsProvided && !parseDeps(input, request.deps, request.depCount, error, false)) {
        return false;
    }
    // tm1637 has nothing else that would populate deps: metric-source dependencies come solely
    // from the (possibly updated) layout's placeholders, same as at create time.
    if (!parseAndEncodeLayoutRequest(input, runtime.deviceId(), request.persistedStateBlob, request.deps, request.depCount, error,
                                     kInvalidLayoutError, kLayoutSizeError, kLayoutDependencyCountError)) {
        return false;
    }
    request.depsProvided = explicitDepsProvided || request.depCount > 0U;
    request.persistedStateProvided = !request.persistedStateBlob.empty();
    return true;
}

DeviceValidationResult Tm1637DeviceApiAdapter::validateDeps(const DeviceRegistry& registry, const DeviceDependencyLink* deps,
                                                            const uint8_t depCount, const IDeviceRuntime* ignoreRuntime) {
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

DeviceValidationResult Tm1637DeviceApiAdapter::validateCreateRequest(const DeviceCreateRequest& request,
                                                                     const DeviceRegistry& registry) const {
    Tm1637DeviceConfigV2 config{};
    if (!decodeTm1637DeviceConfig(request.configBlob.data(), request.configBlob.size(), config)) {
        return {DeviceError::InvalidConfig, kInvalidConfigError};
    }
    const DeviceValidationResult depResult = validateDeps(registry, request.deps.data(), request.depCount, nullptr);
    return depResult.ok() ? config.validate() : depResult;
}

DeviceValidationResult Tm1637DeviceApiAdapter::validateCreateRequest(const DeviceCreateRequest& request,
                                                                     const DeviceCreatePersistenceRequest& persistedRequest,
                                                                     const DeviceRegistry& registry) const {
    const DeviceValidationResult baseResult = validateCreateRequest(request, registry);
    return baseResult.ok() ? validatePersistedLayout(persistedRequest.persistedStateBlob, persistedRequest.persistedStateProvided, registry)
                           : baseResult;
}

DeviceValidationResult Tm1637DeviceApiAdapter::validateUpdateConfigRequest(const IDeviceRuntime& runtime,
                                                                           const DeviceConfigUpdateRequest& request,
                                                                           const DeviceRegistry& registry) const {
    Tm1637DeviceConfigV2 config{};
    if (!decodeTm1637DeviceConfig(request.configBlob.data(), request.configBlob.size(), config)) {
        return {DeviceError::InvalidConfig, kInvalidConfigError};
    }
    const DeviceDependencyLink* deps = request.depsProvided ? request.deps.data() : runtime.dependencyLinks();
    const uint8_t depCount = request.depsProvided ? request.depCount : runtime.dependencyCount();
    const DeviceValidationResult depResult = validateDeps(registry, deps, depCount, &runtime);
    return depResult.ok() ? validatePersistedLayout(request.persistedStateBlob, request.persistedStateProvided, registry) : depResult;
}

DeviceValidationResult Tm1637DeviceApiAdapter::validateSetDepsRequest(const IDeviceRuntime& runtime,
                                                                      const std::array<DeviceDependencyLink, kMaxDeviceDependencies>& deps,
                                                                      uint8_t depCount, const DeviceRegistry& registry) const {
    return validateDeps(registry, deps.data(), depCount, &runtime);
}

const Tm1637Device& Tm1637DeviceApiAdapter::device(const IDeviceRuntime& runtime) {
    return static_cast<const Tm1637Device&>(runtime);
}

void Tm1637DeviceApiAdapter::writeDeviceJson(const IDeviceRuntime& runtime, const DeviceStatus effectiveStatus, JsonObject output) const {
    writeCommonDeviceJson(runtime, effectiveStatus, typeName(), output);
    JsonObject configJson = output["config"].as<JsonObject>();
    device(runtime).config().writeJson(configJson);
    writeDisplayLayoutProfileJson(device(runtime).displayProfile(),
                                  output["runtime"].as<JsonObject>().createNestedObject("displayProfile"));
}

void Tm1637DeviceApiAdapter::writeConfigJson(const IDeviceRuntime& runtime, JsonObject config) const {
    device(runtime).config().writeJson(config);
}

DeviceValidationResult Tm1637DeviceApiAdapter::validatePersistedLayout(const BoundedBlob<kMaxDisplayLayoutBytes>& blob, const bool provided,
                                                                       const DeviceRegistry& registry) {
    if (!provided) {
        return {};
    }
    DisplayLayoutRecordV1 layout{};
    if (!decodeDisplayLayoutBinary(blob.data(), blob.size(), layout) || !validateDisplayLayout(layout, layoutProfile()).ok()) {
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

} // namespace ewfm
