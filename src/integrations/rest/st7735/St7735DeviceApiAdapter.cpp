#include "integrations/rest/st7735/St7735DeviceApiAdapter.h"

#include "devices/bus/spi/SpiBusDevice.h"
#include "devices/core/DeviceBaseConfig.h"
#include "devices/display/DisplayLayoutCodec.h"
#include "devices/display/DisplayTextPlaceholderAst.h"

#include <ArduinoJson.h>

namespace ewfm {
namespace {
DeviceValidationResult validateSpiBusDependency(const DeviceRegistry& registry, DeviceId busDeviceId, uint8_t chipSelectPin,
                                                const IDeviceRuntime* ignoreDependent) {
    const IDeviceRuntime* busRuntime = registry.runtime(busDeviceId);
    if (busRuntime == nullptr || busRuntime->typeId() != SpiBusDevice::descriptor().typeId) {
        return {DeviceError::InvalidRelationship, "st7735 spi bus dependency is missing or invalid"};
    }
    if (busRuntime->hasDuplicateDependentSpiChipSelect(chipSelectPin, ignoreDependent)) {
        return {DeviceError::InvalidRelationship, "duplicate st7735 chip select pin on dependency"};
    }
    return {};
}

bool encodeLayoutRequest(const JsonObjectConst& input, DeviceId deviceId, BoundedBlob<kMaxDisplayLayoutBytes>& blob, const char*& error) {
    const JsonObjectConst layoutInput = input["config"]["layout"].as<JsonObjectConst>();
    if (layoutInput.isNull()) {
        blob.clear();
        error = nullptr;
        return true;
    }

    DisplayLayoutRecordV1 layout{};
    if (!parseDisplayLayoutJson(layoutInput, layout)) {
        error = "st7735 layout is invalid";
        return false;
    }
    layout.deviceId = deviceId;

    std::vector<uint8_t> encoded;
    if (!encodeDisplayLayoutBinary(layout, encoded)) {
        error = "st7735 layout is invalid";
        return false;
    }
    if (!blob.assign(encoded)) {
        error = "st7735 layout exceeds supported size";
        return false;
    }
    error = nullptr;
    return true;
}

bool appendMetricSourceDependency(std::array<DeviceDependencyLink, kMaxDeviceDependencies>& deps, uint8_t& depCount, DeviceId sourceId,
                                  const char*& error) {
    if (sourceId == 0U) {
        return true;
    }
    for (uint8_t index = 0; index < depCount; ++index) {
        if (deps[index].role == DeviceRole::MetricSource && deps[index].deviceId == sourceId) {
            return true;
        }
    }
    if (depCount >= kMaxDeviceDependencies) {
        error = "st7735 layout exceeds supported dependency count";
        return false;
    }
    deps[depCount++] = DeviceDependencyLink{DeviceRole::MetricSource, sourceId};
    return true;
}

bool collectLayoutMetricSourceDependencies(const DisplayLayoutRecordV1& layout,
                                           std::array<DeviceDependencyLink, kMaxDeviceDependencies>& deps, uint8_t& depCount,
                                           const char*& error) {
    for (const DisplayLayoutPageV1& page : layout.pages) {
        for (const DisplayLayoutWidgetV1& widget : page.widgets) {
            if (static_cast<DisplayLayoutWidgetType>(widget.type) != DisplayLayoutWidgetType::Text) {
                continue;
            }
            const DisplayTextCompileResult compiled = compileDisplayTextWidget(widget.text);
            if (!compiled.ok()) {
                error = "st7735 layout is invalid";
                return false;
            }
            for (const DisplayTextSegment& segment : compiled.compiled.segments) {
                if (!segment.placeholder || segment.reference.ns != MetricNamespace::Device) {
                    continue;
                }
                if (!appendMetricSourceDependency(deps, depCount, segment.reference.sourceId, error)) {
                    return false;
                }
            }
        }
    }
    return true;
}

bool validateLayoutMetricPlaceholders(const DisplayLayoutRecordV1& layout, const DeviceRegistry& registry) {
    for (const DisplayLayoutPageV1& page : layout.pages) {
        for (const DisplayLayoutWidgetV1& widget : page.widgets) {
            if (static_cast<DisplayLayoutWidgetType>(widget.type) != DisplayLayoutWidgetType::Text) {
                continue;
            }
            const DeviceValidationResult validation = validateDisplayTextWidget(widget, registry);
            if (!validation.ok()) {
                return false;
            }
        }
    }
    return true;
}
} // namespace

const St7735DeviceApiAdapter& St7735DeviceApiAdapter::instance() {
    static const St7735DeviceApiAdapter adapter;
    return adapter;
}

DeviceTypeId St7735DeviceApiAdapter::typeId() const {
    return St7735Device::descriptor().typeId;
}

const char* St7735DeviceApiAdapter::typeName() const {
    return "st7735";
}

bool St7735DeviceApiAdapter::parseCreateRequest(const JsonObjectConst& input, DeviceCreateRequest& request, const char*& error) const {
    request = {};
    request.typeId = typeId();
    request.configVersion = St7735Device::descriptor().currentConfigVersion;
    const JsonObjectConst configInput = input["config"].as<JsonObjectConst>();
    if (configInput.isNull()) {
        error = "st7735 config is required";
        return false;
    }
    St7735DeviceConfigV4 config{};
    if (!config.parseJson(configInput, error)) {
        return false;
    }
    request.name = config.name;
    request.enabled = config.enabled != 0U;
    request.deps[0] = DeviceDependencyLink{DeviceRole::SpiBus, config.spiBusDeviceId};
    request.depCount = 1U;
    if (!config.validate().ok()) {
        error = "st7735 config is invalid";
        return false;
    }
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    const size_t size = st7735DeviceConfigSize(config);
    if (!encodeSt7735DeviceConfig(config, buffer, size) || !request.configBlob.assign(buffer, size)) {
        error = "failed to encode st7735 config";
        return false;
    }
    return true;
}

bool St7735DeviceApiAdapter::parseCreatePersistedStateRequest(const JsonObjectConst& input, DeviceCreateRequest& request,
                                                              DeviceCreatePersistenceRequest& persistedRequest, const char*& error) const {
    persistedRequest = {};
    if (!encodeLayoutRequest(input, 0, persistedRequest.persistedStateBlob, error)) {
        return false;
    }
    const JsonObjectConst layoutInput = input["config"]["layout"].as<JsonObjectConst>();
    if (!layoutInput.isNull()) {
        DisplayLayoutRecordV1 layout{};
        if (!parseDisplayLayoutJson(layoutInput, layout)) {
            error = "st7735 layout is invalid";
            return false;
        }
        if (!collectLayoutMetricSourceDependencies(layout, request.deps, request.depCount, error)) {
            return false;
        }
    }
    persistedRequest.persistedStateProvided = !persistedRequest.persistedStateBlob.empty();
    return true;
}

DeviceValidationResult St7735DeviceApiAdapter::validateCreateRequest(const DeviceCreateRequest& request,
                                                                     const DeviceRegistry& registry) const {
    if (request.dependencyCount() < 1U || request.dependencyLinks() == nullptr || request.dependencyLinks()[0].deviceId == 0U ||
        request.dependencyLinks()[0].role != DeviceRole::SpiBus) {
        return {DeviceError::InvalidRelationship, "st7735 requires spi bus dependency"};
    }
    for (uint8_t index = 1; index < request.dependencyCount(); ++index) {
        if (request.dependencyLinks()[index].role != DeviceRole::MetricSource || request.dependencyLinks()[index].deviceId == 0U) {
            return {DeviceError::InvalidRelationship, "st7735 metric source dependency is invalid"};
        }
    }

    St7735DeviceConfigV4 config{};
    if (!decodeSt7735DeviceConfig(reinterpret_cast<const uint8_t*>(request.configBlob.data()), request.configBlob.size(), config)) {
        return {DeviceError::InvalidConfig, "st7735 config is invalid"};
    }
    return validateSpiBusDependency(registry, request.dependencyLinks()[0].deviceId, config.chipSelectPin, nullptr);
}

DeviceValidationResult St7735DeviceApiAdapter::validateCreateRequest(const DeviceCreateRequest& request,
                                                                     const DeviceCreatePersistenceRequest& persistedRequest,
                                                                     const DeviceRegistry& registry) const {
    const DeviceValidationResult baseResult = validateCreateRequest(request, registry);
    if (!baseResult.ok()) {
        return baseResult;
    }
    if (persistedRequest.persistedStateProvided) {
        DisplayLayoutRecordV1 layout{};
        if (!decodeDisplayLayoutBinary(persistedRequest.persistedStateBlob.data(), persistedRequest.persistedStateBlob.size(), layout)) {
            return {DeviceError::InvalidConfig, "st7735 layout is invalid"};
        }
        if (!validateLayoutMetricPlaceholders(layout, registry)) {
            return {DeviceError::InvalidRelationship, "st7735 layout placeholder is invalid"};
        }
    }
    return {};
}

bool St7735DeviceApiAdapter::parseUpdateConfigRequest(const JsonObjectConst& input, IDeviceRuntime& runtime,
                                                      DeviceConfigUpdateRequest& request, const char*& error) const {
    const JsonObjectConst configInput = input["config"].as<JsonObjectConst>();
    if (configInput.isNull()) {
        error = "st7735 config is required";
        return false;
    }
    St7735DeviceConfigV4 config = static_cast<const St7735Device&>(runtime).config();
    if (!config.parseJson(configInput, error)) {
        return false;
    }
    if (!config.validate().ok()) {
        error = "st7735 config is invalid";
        return false;
    }
    request = {};
    request.configVersion = St7735Device::descriptor().currentConfigVersion;
    request.enabled = config.enabled != 0U;
    if (!copyBoundedText(request.name, config.name)) {
        error = "device base config is invalid";
        return false;
    }
    request.deps[0] = DeviceDependencyLink{DeviceRole::SpiBus, runtime.dependencyDeviceId(DeviceRole::SpiBus)};
    request.depCount = 1U;
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    const size_t size = st7735DeviceConfigSize(config);
    if (!encodeSt7735DeviceConfig(config, buffer, size) || !request.configBlob.assign(buffer, size)) {
        error = "failed to encode st7735 config";
        return false;
    }
    if (!encodeLayoutRequest(input, runtime.deviceId(), request.persistedStateBlob, error)) {
        return false;
    }
    const JsonObjectConst layoutInput = input["config"]["layout"].as<JsonObjectConst>();
    if (!layoutInput.isNull()) {
        DisplayLayoutRecordV1 layout{};
        if (!parseDisplayLayoutJson(layoutInput, layout)) {
            error = "st7735 layout is invalid";
            return false;
        }
        if (!collectLayoutMetricSourceDependencies(layout, request.deps, request.depCount, error)) {
            return false;
        }
        request.depsProvided = request.depCount > 1U;
    }
    request.persistedStateProvided = !request.persistedStateBlob.empty();
    return true;
}

DeviceValidationResult St7735DeviceApiAdapter::validateUpdateConfigRequest(const IDeviceRuntime& runtime,
                                                                           const DeviceConfigUpdateRequest& request,
                                                                           const DeviceRegistry& registry) const {
    St7735DeviceConfigV4 config{};
    if (!decodeSt7735DeviceConfig(reinterpret_cast<const uint8_t*>(request.configBlob.data()), request.configBlob.size(), config)) {
        return {DeviceError::InvalidConfig, "st7735 config is invalid"};
    }

    DeviceId busDeviceId = runtime.dependencyDeviceId(DeviceRole::SpiBus);
    if (request.depsProvided) {
        if (request.depCount < 1U || request.deps[0].role != DeviceRole::SpiBus || request.deps[0].deviceId == 0U) {
            return {DeviceError::InvalidRelationship, "st7735 requires spi bus dependency"};
        }
        for (uint8_t index = 1; index < request.depCount; ++index) {
            if (request.deps[index].role != DeviceRole::MetricSource || request.deps[index].deviceId == 0U) {
                return {DeviceError::InvalidRelationship, "st7735 metric source dependency is invalid"};
            }
        }
        busDeviceId = request.deps[0].deviceId;
    }
    const DeviceValidationResult busResult = validateSpiBusDependency(registry, busDeviceId, config.chipSelectPin, &runtime);
    if (!busResult.ok()) {
        return busResult;
    }
    if (request.persistedStateProvided) {
        DisplayLayoutRecordV1 layout{};
        if (!decodeDisplayLayoutBinary(request.persistedStateBlob.data(), request.persistedStateBlob.size(), layout)) {
            return {DeviceError::InvalidConfig, "st7735 layout is invalid"};
        }
        if (!validateLayoutMetricPlaceholders(layout, registry)) {
            return {DeviceError::InvalidRelationship, "st7735 layout placeholder is invalid"};
        }
    }
    return {};
}

DeviceValidationResult St7735DeviceApiAdapter::validateSetDepsRequest(const IDeviceRuntime& runtime,
                                                                      const std::array<DeviceDependencyLink, kMaxDeviceDependencies>& deps,
                                                                      uint8_t depCount, const DeviceRegistry& registry) const {
    if (depCount < 1U || deps[0].role != DeviceRole::SpiBus || deps[0].deviceId == 0U) {
        return {DeviceError::InvalidRelationship, "st7735 requires spi bus dependency"};
    }
    for (uint8_t index = 1; index < depCount; ++index) {
        if (deps[index].role != DeviceRole::MetricSource || deps[index].deviceId == 0U) {
            return {DeviceError::InvalidRelationship, "st7735 metric source dependency is invalid"};
        }
    }
    const St7735Device& device = static_cast<const St7735Device&>(runtime);
    return validateSpiBusDependency(registry, deps[0].deviceId, device.config().chipSelectPin, &runtime);
}

void St7735DeviceApiAdapter::writeDeviceJson(const IDeviceRuntime& runtime, const DeviceStatus effectiveStatus, JsonObject output) const {
    writeCommonDeviceJson(runtime, effectiveStatus, typeName(), output);
    const St7735Device& device = static_cast<const St7735Device&>(runtime);
    JsonObject config = output["config"].as<JsonObject>();
    device.config().writeJson(config);
    JsonObject layout = config.createNestedObject("layout");
    writeDisplayLayoutJson(device.layout(), layout);
}

} // namespace ewfm
