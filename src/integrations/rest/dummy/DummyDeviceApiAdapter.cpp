#include "integrations/rest/dummy/DummyDeviceApiAdapter.h"

#include "devices/core/DeviceBaseConfig.h"
#include "devices/dummy/DummyDevice.h"

namespace ewfm {

const DummyDeviceApiAdapter& DummyDeviceApiAdapter::instance() {
    static const DummyDeviceApiAdapter adapter;
    return adapter;
}

DeviceTypeId DummyDeviceApiAdapter::typeId() const {
    return DummyDevice::descriptor().typeId;
}

const char* DummyDeviceApiAdapter::typeName() const {
    return "dummy";
}

bool DummyDeviceApiAdapter::parseCreateRequest(const JsonObjectConst& input, DeviceCreateRequest& request, const char*& error) const {
    request = {};
    request.typeId = typeId();
    request.configVersion = DummyDevice::descriptor().currentConfigVersion;

    const JsonObjectConst configInput = input["config"].as<JsonObjectConst>();
    if (configInput.isNull()) {
        error = "dummy config is required";
        return false;
    }
    DummyDeviceConfigV1 config{};
    if (!config.parseJson(configInput, error)) {
        return false;
    }
    request.name = config.name;
    request.enabled = config.enabled != 0U;

    const uint32_t configVersion = input["configVersion"] | request.configVersion;
    request.configVersion = configVersion;
    if (configVersion != 1U) {
        error = "unsupported DummyDevice config version";
        return false;
    }

    uint8_t buffer[kMaxDeviceConfigBytes]{};
    const size_t size = dummyDeviceConfigSize(config);
    if (!encodeDummyDeviceConfig(config, buffer, size) || !request.configBlob.assign(buffer, size)) {
        error = "failed to encode dummy config";
        return false;
    }
    return true;
}

void DummyDeviceApiAdapter::writeDeviceJson(const IDeviceRuntime& runtime, const DeviceStatus effectiveStatus, JsonObject output) const {
    writeCommonDeviceJson(runtime, effectiveStatus, typeName(), output);
    const DummyDevice& device = static_cast<const DummyDevice&>(runtime);
    JsonObject config = output["config"].as<JsonObject>();
    device.config().writeJson(config);
}

} // namespace ewfm
