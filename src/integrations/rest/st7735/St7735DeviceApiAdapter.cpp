#include "integrations/rest/st7735/St7735DeviceApiAdapter.h"

#include "devices/bus/spi/SpiBusDevice.h"
#include "devices/core/DeviceBaseConfig.h"
#include "devices/display/DisplayLayoutCodec.h"

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

bool encodeLayoutRequest(const JsonObjectConst& input, DeviceId deviceId, BoundedBlob<kMaxDeviceConfigBytes>& blob, const char*& error) {
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
    St7735DeviceConfigV1 config{};
    if (!config.parseJson(configInput, error)) {
        return false;
    }
    request.name = config.name;
    request.enabled = config.enabled != 0U;
    request.deps[0] = DeviceDependencyLink{DeviceDependencyRole::SpiBus, config.spiBusDeviceId};
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

bool St7735DeviceApiAdapter::parseCreatePersistedStateRequest(const JsonObjectConst& input, const DeviceCreateRequest& request,
                                                              DeviceCreatePersistenceRequest& persistedRequest, const char*& error) const {
    (void)request;
    persistedRequest = {};
    if (!encodeLayoutRequest(input, 0, persistedRequest.persistedStateBlob, error)) {
        return false;
    }
    persistedRequest.persistedStateProvided = !persistedRequest.persistedStateBlob.empty();
    return true;
}

DeviceValidationResult St7735DeviceApiAdapter::validateCreateRequest(const DeviceCreateRequest& request,
                                                                     const DeviceRegistry& registry) const {
    if (request.dependencyCount() != 1U || request.dependencyLinks() == nullptr || request.dependencyLinks()[0].deviceId == 0U ||
        request.dependencyLinks()[0].role != DeviceDependencyRole::SpiBus) {
        return {DeviceError::InvalidRelationship, "st7735 requires spi bus dependency"};
    }

    St7735DeviceConfigV1 config{};
    if (!decodeSt7735DeviceConfig(reinterpret_cast<const uint8_t*>(request.configBlob.data()), request.configBlob.size(), config)) {
        return {DeviceError::InvalidConfig, "st7735 config is invalid"};
    }
    return validateSpiBusDependency(registry, request.dependencyLinks()[0].deviceId, config.chipSelectPin, nullptr);
}

bool St7735DeviceApiAdapter::parseUpdateConfigRequest(const JsonObjectConst& input, IDeviceRuntime& runtime,
                                                      DeviceConfigUpdateRequest& request, const char*& error) const {
    const JsonObjectConst configInput = input["config"].as<JsonObjectConst>();
    if (configInput.isNull()) {
        error = "st7735 config is required";
        return false;
    }
    St7735DeviceConfigV1 config{};
    if (!config.parseJson(configInput, error)) {
        return false;
    }
    config.enabled = runtime.enabled() ? 1U : 0U;
    if (!copyBoundedText(config.name, runtime.name())) {
        error = "device base config is invalid";
        return false;
    }
    if (!config.validate().ok()) {
        error = "st7735 config is invalid";
        return false;
    }
    request = {};
    request.configVersion = St7735Device::descriptor().currentConfigVersion;
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    const size_t size = st7735DeviceConfigSize(config);
    if (!encodeSt7735DeviceConfig(config, buffer, size) || !request.configBlob.assign(buffer, size)) {
        error = "failed to encode st7735 config";
        return false;
    }
    if (!encodeLayoutRequest(input, runtime.deviceId(), request.persistedStateBlob, error)) {
        return false;
    }
    request.persistedStateProvided = !request.persistedStateBlob.empty();
    return true;
}

DeviceValidationResult St7735DeviceApiAdapter::validateUpdateConfigRequest(const IDeviceRuntime& runtime,
                                                                           const DeviceConfigUpdateRequest& request,
                                                                           const DeviceRegistry& registry) const {
    St7735DeviceConfigV1 config{};
    if (!decodeSt7735DeviceConfig(reinterpret_cast<const uint8_t*>(request.configBlob.data()), request.configBlob.size(), config)) {
        return {DeviceError::InvalidConfig, "st7735 config is invalid"};
    }

    DeviceId busDeviceId = runtime.dependencyDeviceId(DeviceDependencyRole::SpiBus);
    if (request.depsProvided) {
        if (request.depCount != 1U || request.deps[0].role != DeviceDependencyRole::SpiBus || request.deps[0].deviceId == 0U) {
            return {DeviceError::InvalidRelationship, "st7735 requires spi bus dependency"};
        }
        busDeviceId = request.deps[0].deviceId;
    }

    return validateSpiBusDependency(registry, busDeviceId, config.chipSelectPin, &runtime);
}

DeviceValidationResult St7735DeviceApiAdapter::validateSetDepsRequest(const IDeviceRuntime& runtime,
                                                                      const std::array<DeviceDependencyLink, kMaxDeviceDependencies>& deps,
                                                                      uint8_t depCount, const DeviceRegistry& registry) const {
    if (depCount != 1U || deps[0].role != DeviceDependencyRole::SpiBus || deps[0].deviceId == 0U) {
        return {DeviceError::InvalidRelationship, "st7735 requires spi bus dependency"};
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
