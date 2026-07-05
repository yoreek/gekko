#include "integrations/rest/spi_bus/SpiBusDeviceApiAdapter.h"

#include "devices/bus/spi/SpiBusDevice.h"
#include "devices/core/DeviceBaseConfig.h"

namespace ewfm {

const SpiBusDeviceApiAdapter& SpiBusDeviceApiAdapter::instance() {
    static const SpiBusDeviceApiAdapter adapter;
    return adapter;
}

DeviceTypeId SpiBusDeviceApiAdapter::typeId() const {
    return SpiBusDevice::descriptor().typeId;
}

const char* SpiBusDeviceApiAdapter::typeName() const {
    return "spi_bus";
}

bool SpiBusDeviceApiAdapter::parseCreateRequest(const JsonObjectConst& input, DeviceCreateRequest& request, const char*& error) const {
    request = {};
    request.typeId = typeId();
    request.configVersion = SpiBusDevice::descriptor().currentConfigVersion;

    const JsonObjectConst configInput = input["config"].as<JsonObjectConst>();
    if (configInput.isNull()) {
        error = "spi bus config is required";
        return false;
    }

    SpiBusDeviceConfigV1 config{};
    if (!config.parseJson(configInput, error)) {
        return false;
    }
    request.name = config.name;
    request.enabled = config.enabled != 0U;
    if (!config.validate().ok()) {
        error = "spi bus config is invalid";
        return false;
    }
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    const size_t size = spiBusDeviceConfigSize(config);
    if (!encodeSpiBusDeviceConfig(config, buffer, size) || !request.configBlob.assign(buffer, size)) {
        error = "failed to encode spi bus config";
        return false;
    }
    return true;
}

bool SpiBusDeviceApiAdapter::parseUpdateConfigRequest(const JsonObjectConst& input, IDeviceRuntime& runtime,
                                                      DeviceConfigUpdateRequest& request, const char*& error) const {
    const JsonObjectConst configInput = input["config"].as<JsonObjectConst>();
    if (configInput.isNull()) {
        error = "spi bus config is required";
        return false;
    }

    SpiBusDeviceConfigV1 config = static_cast<const SpiBusDevice&>(runtime).config();
    if (!config.parseJson(configInput, error)) {
        return false;
    }
    config.enabled = runtime.enabled() ? 1U : 0U;
    if (!copyBoundedText(config.name, runtime.name())) {
        error = "device base config is invalid";
        return false;
    }
    if (!config.validate().ok()) {
        error = "spi bus config is invalid";
        return false;
    }
    request = {};
    request.configVersion = SpiBusDevice::descriptor().currentConfigVersion;
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    const size_t size = spiBusDeviceConfigSize(config);
    if (!encodeSpiBusDeviceConfig(config, buffer, size) || !request.configBlob.assign(buffer, size)) {
        error = "failed to encode spi bus config";
        return false;
    }
    return true;
}

void SpiBusDeviceApiAdapter::writeDeviceJson(const IDeviceRuntime& runtime, const DeviceStatus effectiveStatus, JsonObject output) const {
    writeCommonDeviceJson(runtime, effectiveStatus, typeName(), output);
    const SpiBusDevice& device = static_cast<const SpiBusDevice&>(runtime);
    device.writeDeviceJson(output);
}

} // namespace ewfm
