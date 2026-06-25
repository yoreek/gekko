#include "integrations/rest/oled_display/OledDisplayDeviceApiAdapter.h"

#include "devices/bus/i2c/I2cBusDevice.h"
#include "devices/core/DeviceBaseConfig.h"
#include "devices/display/oled/OledDisplayDevice.h"
#include "devices/display/oled/OledDisplayLayoutCodec.h"

#include <ArduinoJson.h>

namespace ewfm {
namespace {
DeviceValidationResult validateI2cBusDependency(const DeviceRegistry& registry, DeviceId busDeviceId, uint8_t i2cAddress,
                                                const IDeviceRuntime* ignoreDependent) {
    const IDeviceRuntime* busRuntime = registry.runtime(busDeviceId);
    if (busRuntime == nullptr || busRuntime->typeId() != I2cBusDevice::descriptor().typeId) {
        return {DeviceError::InvalidRelationship, "oled display i2c bus dependency is missing or invalid"};
    }
    if (busRuntime->hasDuplicateDependentI2cAddress(i2cAddress, ignoreDependent)) {
        return {DeviceError::InvalidRelationship, "duplicate oled display i2c address on dependency"};
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

    OledDisplayLayoutRecordV1 layout{};
    if (!parseOledDisplayLayoutJson(layoutInput, layout)) {
        error = "oled display layout is invalid";
        return false;
    }
    layout.deviceId = deviceId;

    std::vector<uint8_t> encoded;
    if (!encodeOledDisplayLayoutBinary(layout, encoded)) {
        error = "oled display layout is invalid";
        return false;
    }
    if (!blob.assign(encoded)) {
        error = "oled display layout exceeds supported size";
        return false;
    }
    error = nullptr;
    return true;
}
} // namespace

const OledDisplayDeviceApiAdapter& OledDisplayDeviceApiAdapter::instance() {
    static const OledDisplayDeviceApiAdapter adapter;
    return adapter;
}

DeviceTypeId OledDisplayDeviceApiAdapter::typeId() const {
    return OledDisplayDevice::descriptor().typeId;
}

const char* OledDisplayDeviceApiAdapter::typeName() const {
    return "oled_display";
}

bool OledDisplayDeviceApiAdapter::parseCreateRequest(const JsonObjectConst& input, DeviceCreateRequest& request, const char*& error) const {
    request = {};
    request.typeId = typeId();
    request.configVersion = OledDisplayDevice::descriptor().currentConfigVersion;
    const JsonObjectConst configInput = input["config"].as<JsonObjectConst>();
    if (configInput.isNull()) {
        error = "oled display config is required";
        return false;
    }
    OledDisplayDeviceConfigV1 config{};
    if (!config.parseJson(configInput, error)) {
        return false;
    }
    request.name = config.name;
    request.enabled = config.enabled != 0U;
    request.deps[0] = DeviceDependencyLink{DeviceDependencyRole::I2CBus, config.i2cBusDeviceId};
    request.depCount = 1U;
    if (!config.validate().ok()) {
        error = "oled display config is invalid";
        return false;
    }
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    const size_t size = oledDisplayDeviceConfigSize(config);
    if (!encodeOledDisplayDeviceConfig(config, buffer, size) || !request.configBlob.assign(buffer, size)) {
        error = "failed to encode oled display config";
        return false;
    }
    return true;
}

bool OledDisplayDeviceApiAdapter::parseCreatePersistedStateRequest(const JsonObjectConst& input, const DeviceCreateRequest& request,
                                                                   DeviceCreatePersistenceRequest& persistedRequest,
                                                                   const char*& error) const {
    (void)request;
    persistedRequest = {};
    if (!encodeLayoutRequest(input, 0, persistedRequest.persistedStateBlob, error)) {
        return false;
    }
    persistedRequest.persistedStateProvided = !persistedRequest.persistedStateBlob.empty();
    return true;
}

DeviceValidationResult OledDisplayDeviceApiAdapter::validateCreateRequest(const DeviceCreateRequest& request,
                                                                          const DeviceRegistry& registry) const {
    if (request.dependencyCount() != 1U || request.dependencyLinks() == nullptr || request.dependencyLinks()[0].deviceId == 0U ||
        request.dependencyLinks()[0].role != DeviceDependencyRole::I2CBus) {
        return {DeviceError::InvalidRelationship, "oled display requires i2c bus dependency"};
    }

    OledDisplayDeviceConfigV1 config{};
    if (!decodeOledDisplayDeviceConfig(reinterpret_cast<const uint8_t*>(request.configBlob.data()), request.configBlob.size(), config)) {
        return {DeviceError::InvalidConfig, "oled display config is invalid"};
    }
    return validateI2cBusDependency(registry, request.dependencyLinks()[0].deviceId, config.i2cAddress, nullptr);
}

bool OledDisplayDeviceApiAdapter::parseUpdateConfigRequest(const JsonObjectConst& input, IDeviceRuntime& runtime,
                                                           DeviceConfigUpdateRequest& request, const char*& error) const {
    const JsonObjectConst configInput = input["config"].as<JsonObjectConst>();
    if (configInput.isNull()) {
        error = "oled display config is required";
        return false;
    }
    OledDisplayDeviceConfigV1 config{};
    if (!config.parseJson(configInput, error)) {
        return false;
    }
    config.enabled = runtime.enabled() ? 1U : 0U;
    if (!copyBoundedText(config.name, runtime.name())) {
        error = "device base config is invalid";
        return false;
    }
    if (!config.validate().ok()) {
        error = "oled display config is invalid";
        return false;
    }
    request = {};
    request.configVersion = OledDisplayDevice::descriptor().currentConfigVersion;
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    const size_t size = oledDisplayDeviceConfigSize(config);
    if (!encodeOledDisplayDeviceConfig(config, buffer, size) || !request.configBlob.assign(buffer, size)) {
        error = "failed to encode oled display config";
        return false;
    }
    if (!encodeLayoutRequest(input, runtime.deviceId(), request.persistedStateBlob, error)) {
        return false;
    }
    request.persistedStateProvided = !request.persistedStateBlob.empty();
    return true;
}

DeviceValidationResult OledDisplayDeviceApiAdapter::validateUpdateConfigRequest(const IDeviceRuntime& runtime,
                                                                                const DeviceConfigUpdateRequest& request,
                                                                                const DeviceRegistry& registry) const {
    OledDisplayDeviceConfigV1 config{};
    if (!decodeOledDisplayDeviceConfig(reinterpret_cast<const uint8_t*>(request.configBlob.data()), request.configBlob.size(), config)) {
        return {DeviceError::InvalidConfig, "oled display config is invalid"};
    }

    DeviceId busDeviceId = runtime.dependencyDeviceId(DeviceDependencyRole::I2CBus);
    if (request.depsProvided) {
        if (request.depCount != 1U || request.deps[0].role != DeviceDependencyRole::I2CBus || request.deps[0].deviceId == 0U) {
            return {DeviceError::InvalidRelationship, "oled display requires i2c bus dependency"};
        }
        busDeviceId = request.deps[0].deviceId;
    }

    return validateI2cBusDependency(registry, busDeviceId, config.i2cAddress, &runtime);
}

DeviceValidationResult
OledDisplayDeviceApiAdapter::validateSetDepsRequest(const IDeviceRuntime& runtime,
                                                    const std::array<DeviceDependencyLink, kMaxDeviceDependencies>& deps, uint8_t depCount,
                                                    const DeviceRegistry& registry) const {
    if (depCount != 1U || deps[0].role != DeviceDependencyRole::I2CBus || deps[0].deviceId == 0U) {
        return {DeviceError::InvalidRelationship, "oled display requires i2c bus dependency"};
    }
    const OledDisplayDevice& device = static_cast<const OledDisplayDevice&>(runtime);
    return validateI2cBusDependency(registry, deps[0].deviceId, device.config().i2cAddress, &runtime);
}

void OledDisplayDeviceApiAdapter::writeDeviceJson(const IDeviceRuntime& runtime, const DeviceStatus effectiveStatus,
                                                  JsonObject output) const {
    writeCommonDeviceJson(runtime, effectiveStatus, typeName(), output);
    const OledDisplayDevice& device = static_cast<const OledDisplayDevice&>(runtime);
    JsonObject config = output["config"].as<JsonObject>();
    device.config().writeJson(config);
    JsonObject layout = config.createNestedObject("layout");
    writeOledDisplayLayoutJson(device.layout(), layout);
}

} // namespace ewfm
