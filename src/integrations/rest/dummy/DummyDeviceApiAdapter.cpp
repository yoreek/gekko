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

    DeviceBaseConfigV1 base{};
    if (!parseDeviceBaseConfigJson(input, base, error)) {
        return false;
    }
    request.name = base.name;
    request.enabled = base.enabled != 0U;

    const uint32_t configVersion = input["config_version"] | request.configVersion;
    DummyDeviceConfigV1 config = base;
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
    writeDummyDeviceConfigJson(device.config(), config);
}

} // namespace ewfm
