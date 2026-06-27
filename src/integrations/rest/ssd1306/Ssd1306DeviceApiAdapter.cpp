#include "integrations/rest/ssd1306/Ssd1306DeviceApiAdapter.h"

#include "devices/bus/i2c/I2cBusDevice.h"
#include "devices/core/DeviceBaseConfig.h"
#include "devices/display/ssd1306/Ssd1306Device.h"

#include <ArduinoJson.h>

namespace ewfm {
namespace {
DeviceValidationResult validateI2cBusDependency(const DeviceRegistry& registry, DeviceId busDeviceId, uint8_t i2cAddress,
                                                const IDeviceRuntime* ignoreDependent) {
    const IDeviceRuntime* busRuntime = registry.runtime(busDeviceId);
    if (busRuntime == nullptr || busRuntime->typeId() != I2cBusDevice::descriptor().typeId) {
        return {DeviceError::InvalidRelationship, "ssd1306 i2c bus dependency is missing or invalid"};
    }
    if (busRuntime->hasDuplicateDependentI2cAddress(i2cAddress, ignoreDependent)) {
        return {DeviceError::InvalidRelationship, "duplicate ssd1306 i2c address on dependency"};
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
        error = "ssd1306 layout is invalid";
        return false;
    }
    layout.deviceId = deviceId;

    std::vector<uint8_t> encoded;
    if (!encodeDisplayLayoutBinary(layout, encoded)) {
        error = "ssd1306 layout is invalid";
        return false;
    }
    if (!blob.assign(encoded)) {
        error = "ssd1306 layout exceeds supported size";
        return false;
    }
    error = nullptr;
    return true;
}
} // namespace

const Ssd1306DeviceApiAdapter& Ssd1306DeviceApiAdapter::instance() {
    static const Ssd1306DeviceApiAdapter adapter;
    return adapter;
}

DeviceTypeId Ssd1306DeviceApiAdapter::typeId() const {
    return Ssd1306Device::descriptor().typeId;
}

const char* Ssd1306DeviceApiAdapter::typeName() const {
    return "ssd1306";
}

bool Ssd1306DeviceApiAdapter::parseCreateRequest(const JsonObjectConst& input, DeviceCreateRequest& request, const char*& error) const {
    request = {};
    request.typeId = typeId();
    request.configVersion = Ssd1306Device::descriptor().currentConfigVersion;
    const JsonObjectConst configInput = input["config"].as<JsonObjectConst>();
    if (configInput.isNull()) {
        error = "ssd1306 config is required";
        return false;
    }
    Ssd1306DeviceConfigV1 config{};
    if (!config.parseJson(configInput, error)) {
        return false;
    }
    request.name = config.name;
    request.enabled = config.enabled != 0U;
    request.deps[0] = DeviceDependencyLink{DeviceDependencyRole::I2CBus, config.i2cBusDeviceId};
    request.depCount = 1U;
    if (!config.validate().ok()) {
        error = "ssd1306 config is invalid";
        return false;
    }
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    const size_t size = ssd1306DeviceConfigSize(config);
    if (!encodeSsd1306DeviceConfig(config, buffer, size) || !request.configBlob.assign(buffer, size)) {
        error = "failed to encode ssd1306 config";
        return false;
    }
    return true;
}

bool Ssd1306DeviceApiAdapter::parseCreatePersistedStateRequest(const JsonObjectConst& input, const DeviceCreateRequest& request,
                                                               DeviceCreatePersistenceRequest& persistedRequest, const char*& error) const {
    (void)request;
    persistedRequest = {};
    if (!encodeLayoutRequest(input, 0, persistedRequest.persistedStateBlob, error)) {
        return false;
    }
    persistedRequest.persistedStateProvided = !persistedRequest.persistedStateBlob.empty();
    return true;
}

DeviceValidationResult Ssd1306DeviceApiAdapter::validateCreateRequest(const DeviceCreateRequest& request,
                                                                      const DeviceRegistry& registry) const {
    if (request.dependencyCount() != 1U || request.dependencyLinks() == nullptr || request.dependencyLinks()[0].deviceId == 0U ||
        request.dependencyLinks()[0].role != DeviceDependencyRole::I2CBus) {
        return {DeviceError::InvalidRelationship, "ssd1306 requires i2c bus dependency"};
    }

    Ssd1306DeviceConfigV1 config{};
    if (!decodeSsd1306DeviceConfig(reinterpret_cast<const uint8_t*>(request.configBlob.data()), request.configBlob.size(), config)) {
        return {DeviceError::InvalidConfig, "ssd1306 config is invalid"};
    }
    return validateI2cBusDependency(registry, request.dependencyLinks()[0].deviceId, config.i2cAddress, nullptr);
}

bool Ssd1306DeviceApiAdapter::parseUpdateConfigRequest(const JsonObjectConst& input, IDeviceRuntime& runtime,
                                                       DeviceConfigUpdateRequest& request, const char*& error) const {
    const JsonObjectConst configInput = input["config"].as<JsonObjectConst>();
    if (configInput.isNull()) {
        error = "ssd1306 config is required";
        return false;
    }
    Ssd1306DeviceConfigV1 config{};
    if (!config.parseJson(configInput, error)) {
        return false;
    }
    config.enabled = runtime.enabled() ? 1U : 0U;
    if (!copyBoundedText(config.name, runtime.name())) {
        error = "device base config is invalid";
        return false;
    }
    if (!config.validate().ok()) {
        error = "ssd1306 config is invalid";
        return false;
    }
    request = {};
    request.configVersion = Ssd1306Device::descriptor().currentConfigVersion;
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    const size_t size = ssd1306DeviceConfigSize(config);
    if (!encodeSsd1306DeviceConfig(config, buffer, size) || !request.configBlob.assign(buffer, size)) {
        error = "failed to encode ssd1306 config";
        return false;
    }
    if (!encodeLayoutRequest(input, runtime.deviceId(), request.persistedStateBlob, error)) {
        return false;
    }
    request.persistedStateProvided = !request.persistedStateBlob.empty();
    return true;
}

DeviceValidationResult Ssd1306DeviceApiAdapter::validateUpdateConfigRequest(const IDeviceRuntime& runtime,
                                                                            const DeviceConfigUpdateRequest& request,
                                                                            const DeviceRegistry& registry) const {
    Ssd1306DeviceConfigV1 config{};
    if (!decodeSsd1306DeviceConfig(reinterpret_cast<const uint8_t*>(request.configBlob.data()), request.configBlob.size(), config)) {
        return {DeviceError::InvalidConfig, "ssd1306 config is invalid"};
    }

    DeviceId busDeviceId = runtime.dependencyDeviceId(DeviceDependencyRole::I2CBus);
    if (request.depsProvided) {
        if (request.depCount != 1U || request.deps[0].role != DeviceDependencyRole::I2CBus || request.deps[0].deviceId == 0U) {
            return {DeviceError::InvalidRelationship, "ssd1306 requires i2c bus dependency"};
        }
        busDeviceId = request.deps[0].deviceId;
    }

    return validateI2cBusDependency(registry, busDeviceId, config.i2cAddress, &runtime);
}

DeviceValidationResult Ssd1306DeviceApiAdapter::validateSetDepsRequest(const IDeviceRuntime& runtime,
                                                                       const std::array<DeviceDependencyLink, kMaxDeviceDependencies>& deps,
                                                                       uint8_t depCount, const DeviceRegistry& registry) const {
    if (depCount != 1U || deps[0].role != DeviceDependencyRole::I2CBus || deps[0].deviceId == 0U) {
        return {DeviceError::InvalidRelationship, "ssd1306 requires i2c bus dependency"};
    }
    const Ssd1306Device& device = static_cast<const Ssd1306Device&>(runtime);
    return validateI2cBusDependency(registry, deps[0].deviceId, device.config().i2cAddress, &runtime);
}

void Ssd1306DeviceApiAdapter::writeDeviceJson(const IDeviceRuntime& runtime, const DeviceStatus effectiveStatus, JsonObject output) const {
    writeCommonDeviceJson(runtime, effectiveStatus, typeName(), output);
    const Ssd1306Device& device = static_cast<const Ssd1306Device&>(runtime);
    JsonObject config = output["config"].as<JsonObject>();
    device.config().writeJson(config);
    JsonObject layout = config.createNestedObject("layout");
    writeDisplayLayoutJson(device.layout(), layout);
}

} // namespace ewfm
