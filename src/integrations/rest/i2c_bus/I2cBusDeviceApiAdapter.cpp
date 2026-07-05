#include "integrations/rest/i2c_bus/I2cBusDeviceApiAdapter.h"

#include "devices/bus/i2c/I2cBusDevice.h"
#include "devices/core/DeviceBaseConfig.h"

#include <cstdio>

namespace ewfm {

const I2cBusDeviceApiAdapter& I2cBusDeviceApiAdapter::instance() {
    static const I2cBusDeviceApiAdapter adapter;
    return adapter;
}

DeviceTypeId I2cBusDeviceApiAdapter::typeId() const {
    return I2cBusDevice::descriptor().typeId;
}

const char* I2cBusDeviceApiAdapter::typeName() const {
    return "i2c_bus";
}

bool I2cBusDeviceApiAdapter::parseCreateRequest(const JsonObjectConst& input, DeviceCreateRequest& request, const char*& error) const {
    request = {};
    request.typeId = typeId();
    request.configVersion = I2cBusDevice::descriptor().currentConfigVersion;

    const JsonObjectConst configInput = input["config"].as<JsonObjectConst>();
    if (configInput.isNull()) {
        error = "i2c bus config is required";
        return false;
    }

    I2cBusDeviceConfigV1 config{};
    if (!config.parseJson(configInput, error)) {
        return false;
    }
    request.name = config.name;
    request.enabled = config.enabled != 0U;
    if (!config.validate().ok()) {
        error = "i2c bus config is invalid";
        return false;
    }
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    const size_t size = i2cBusDeviceConfigSize(config);
    if (!encodeI2cBusDeviceConfig(config, buffer, size) || !request.configBlob.assign(buffer, size)) {
        error = "failed to encode i2c bus config";
        return false;
    }
    return true;
}

bool I2cBusDeviceApiAdapter::parseUpdateConfigRequest(const JsonObjectConst& input, IDeviceRuntime& runtime,
                                                      DeviceConfigUpdateRequest& request, const char*& error) const {
    const JsonObjectConst configInput = input["config"].as<JsonObjectConst>();
    if (configInput.isNull()) {
        error = "i2c bus config is required";
        return false;
    }

    I2cBusDeviceConfigV1 config = static_cast<const I2cBusDevice&>(runtime).config();
    if (!config.parseJson(configInput, error)) {
        return false;
    }
    if (!config.validate().ok()) {
        error = "i2c bus config is invalid";
        return false;
    }
    request = {};
    request.configVersion = I2cBusDevice::descriptor().currentConfigVersion;
    request.enabled = config.enabled != 0U;
    if (!copyBoundedText(request.name, config.name)) {
        error = "device base config is invalid";
        return false;
    }
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    const size_t size = i2cBusDeviceConfigSize(config);
    if (!encodeI2cBusDeviceConfig(config, buffer, size) || !request.configBlob.assign(buffer, size)) {
        error = "failed to encode i2c bus config";
        return false;
    }
    return true;
}

void I2cBusDeviceApiAdapter::writeDeviceJson(const IDeviceRuntime& runtime, const DeviceStatus effectiveStatus, JsonObject output) const {
    writeCommonDeviceJson(runtime, effectiveStatus, typeName(), output);
    const I2cBusDevice& device = static_cast<const I2cBusDevice&>(runtime);
    JsonObject config = output["config"].as<JsonObject>();
    device.config().writeJson(config);
    JsonObject runtimeJson = output["runtime"].as<JsonObject>();
    runtimeJson["generation"] = device.generation();
    runtimeJson["transactionActive"] = device.dependencyTransactionActive();
    device.diagnostics().writeJson(runtimeJson);
    JsonObject scanJson = runtimeJson.createNestedObject("scan");
    scanJson["inProgress"] = device.scan().inProgress;
    scanJson["ready"] = device.scan().ready;
    scanJson["deviceCount"] = device.scan().deviceCount;
    scanJson["truncated"] = device.scan().truncated;
    scanJson["nextAddress"] = device.scan().nextAddress;
    JsonArray devices = scanJson.createNestedArray("devices");
    for (uint8_t index = 0; index < device.scan().deviceCount; ++index) {
        JsonObject item = devices.createNestedObject();
        char addressHex[8]{};
        std::snprintf(addressHex, sizeof(addressHex), "0x%02X", device.scan().devices[index]);
        item["address"] = device.scan().devices[index];
        item["addressHex"] = addressHex;
    }
}

} // namespace ewfm
