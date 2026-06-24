#include "integrations/rest/gpio_switch/GpioSwitchDeviceApiAdapter.h"

#include "devices/core/DeviceBaseConfig.h"
#include "devices/switch/gpio/GpioSwitchDevice.h"

namespace ewfm {

const GpioSwitchDeviceApiAdapter& GpioSwitchDeviceApiAdapter::instance() {
    static const GpioSwitchDeviceApiAdapter adapter;
    return adapter;
}

DeviceTypeId GpioSwitchDeviceApiAdapter::typeId() const {
    return GpioSwitchDevice::descriptor().typeId;
}

const char* GpioSwitchDeviceApiAdapter::typeName() const {
    return "gpio_switch";
}

bool GpioSwitchDeviceApiAdapter::parseCreateRequest(const JsonObjectConst& input, DeviceCreateRequest& request, const char*& error) const {
    request = {};
    request.typeId = typeId();
    request.configVersion = GpioSwitchDevice::descriptor().currentConfigVersion;

    DeviceBaseConfigV1 base{};
    if (!parseDeviceBaseConfigJson(input, base, error)) {
        return false;
    }
    request.name = base.name;
    request.enabled = base.enabled != 0U;

    const JsonObjectConst configObject = input["config"].as<JsonObjectConst>();
    const JsonObjectConst configInput = configObject.isNull() ? input : configObject;
    GpioSwitchDevicePersistedConfigV1 config{};
    if (!parseGpioSwitchDeviceConfigJson(configInput, config, error)) {
        return false;
    }
    config.switchConfig.base = base;
    if (!validateSwitchDeviceConfig(config.switchConfig).ok()) {
        error = "gpio switch config is invalid";
        return false;
    }
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    const size_t size = gpioSwitchDeviceConfigSize(config);
    if (!encodeGpioSwitchDeviceConfig(config, buffer, size) || !request.configBlob.assign(buffer, size)) {
        error = "failed to encode gpio switch config";
        return false;
    }
    return true;
}

bool GpioSwitchDeviceApiAdapter::parseUpdateConfigRequest(const JsonObjectConst& input, const IDeviceRuntime& runtime,
                                                          DeviceConfigUpdateRequest& request, const char*& error) const {
    const JsonObjectConst configObject = input["config"].as<JsonObjectConst>();
    const JsonObjectConst configInput = configObject.isNull() ? input : configObject;
    if (configObject.isNull() && input["gpioPin"].isNull() && input["restorePreviousState"].isNull() && input["startupState"].isNull() &&
        input["safeState"].isNull() && input["inverted"].isNull()) {
        error = "gpio switch config is required";
        return false;
    }

    DeviceBaseConfigV1 base{};
    base.enabled = runtime.enabled() ? 1U : 0U;
    if (!copyBoundedText(base.name, runtime.name())) {
        error = "device base config is invalid";
        return false;
    }

    GpioSwitchDevicePersistedConfigV1 config{};
    if (!parseGpioSwitchDeviceConfigJson(configInput, config, error)) {
        return false;
    }
    config.switchConfig.base = base;
    if (!validateSwitchDeviceConfig(config.switchConfig).ok()) {
        error = "gpio switch config is invalid";
        return false;
    }

    request = {};
    request.configVersion = GpioSwitchDevice::descriptor().currentConfigVersion;
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    const size_t size = gpioSwitchDeviceConfigSize(config);
    if (!encodeGpioSwitchDeviceConfig(config, buffer, size) || !request.configBlob.assign(buffer, size)) {
        error = "failed to encode gpio switch config";
        return false;
    }
    return true;
}

void GpioSwitchDeviceApiAdapter::writeDeviceJson(const IDeviceRuntime& runtime, const DeviceStatus effectiveStatus,
                                                 JsonObject output) const {
    writeCommonDeviceJson(runtime, effectiveStatus, typeName(), output);
    const GpioSwitchDevice& device = static_cast<const GpioSwitchDevice&>(runtime);
    JsonObject config = output["config"].as<JsonObject>();
    writeGpioSwitchDeviceConfigJson(device.config(), config);
    JsonObject runtimeJson = output["runtime"].as<JsonObject>();
    JsonObject outputJson = runtimeJson.createNestedObject("output");
    outputJson["state"] = outputStateName(device.outputState());
    outputJson["physicalLevel"] = device.physicalOutputState();
}

} // namespace ewfm
