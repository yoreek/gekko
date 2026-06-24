#include "integrations/rest/onewire_bus/OneWireBusDeviceApiAdapter.h"

#include "devices/bus/onewire/OneWireBusDevice.h"
#include "devices/core/DeviceBaseConfig.h"

namespace ewfm {
namespace {
[[maybe_unused]] void writeScanDevice(JsonArray array, const OneWireRomAddress& address) {
    char rom[17]{};
    char family[3]{};
    (void)formatOneWireRomAddress(address, rom);
    family[0] = "0123456789ABCDEF"[(address.bytes[0] >> 4) & 0x0F];
    family[1] = "0123456789ABCDEF"[address.bytes[0] & 0x0F];
    family[2] = '\0';

    JsonObject item = array.createNestedObject();
    item["address"] = rom;
    item["familyCode"] = family;
}
} // namespace

const OneWireBusDeviceApiAdapter& OneWireBusDeviceApiAdapter::instance() {
    static const OneWireBusDeviceApiAdapter adapter;
    return adapter;
}

DeviceTypeId OneWireBusDeviceApiAdapter::typeId() const {
    return OneWireBusDevice::descriptor().typeId;
}

const char* OneWireBusDeviceApiAdapter::typeName() const {
    return "onewire_bus";
}

bool OneWireBusDeviceApiAdapter::parseCreateRequest(const JsonObjectConst& input, DeviceCreateRequest& request, const char*& error) const {
    request = {};
    request.typeId = typeId();
    request.configVersion = OneWireBusDevice::descriptor().currentConfigVersion;

    DeviceBaseConfigV1 base{};
    if (!parseDeviceBaseConfigJson(input, base, error)) {
        return false;
    }
    request.name = base.name;
    request.enabled = base.enabled != 0U;

    const JsonObjectConst configObject = input["config"].as<JsonObjectConst>();
    const JsonObjectConst configInput = configObject.isNull() ? input : configObject;
    OneWireBusDeviceConfigV1 config{};
    if (!parseOneWireBusDeviceConfigJson(configInput, config, error)) {
        return false;
    }
    config.base = base;
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    const size_t size = oneWireBusDeviceConfigSize(config);
    if (!encodeOneWireBusDeviceConfig(config, buffer, size) || !request.configBlob.assign(buffer, size)) {
        error = "failed to encode onewire bus config";
        return false;
    }
    return true;
}

bool OneWireBusDeviceApiAdapter::parseUpdateConfigRequest(const JsonObjectConst& input, const IDeviceRuntime& runtime,
                                                          DeviceConfigUpdateRequest& request, const char*& error) const {
    const JsonObjectConst configObject = input["config"].as<JsonObjectConst>();
    const JsonObjectConst configInput = configObject.isNull() ? input : configObject;
    if (configObject.isNull() && input["gpioPin"].isNull() && input["internalPullup"].isNull()) {
        error = "onewire bus config is required";
        return false;
    }

    DeviceBaseConfigV1 base{};
    base.enabled = runtime.enabled() ? 1U : 0U;
    if (!copyBoundedText(base.name, runtime.name())) {
        error = "device base config is invalid";
        return false;
    }

    OneWireBusDeviceConfigV1 config{};
    if (!parseOneWireBusDeviceConfigJson(configInput, config, error)) {
        return false;
    }
    config.base = base;

    request = {};
    request.configVersion = OneWireBusDevice::descriptor().currentConfigVersion;
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    const size_t size = oneWireBusDeviceConfigSize(config);
    if (!encodeOneWireBusDeviceConfig(config, buffer, size) || !request.configBlob.assign(buffer, size)) {
        error = "failed to encode onewire bus config";
        return false;
    }
    return true;
}

void OneWireBusDeviceApiAdapter::writeDeviceJson(const IDeviceRuntime& runtime, const DeviceStatus effectiveStatus,
                                                 JsonObject output) const {
    writeCommonDeviceJson(runtime, effectiveStatus, typeName(), output);
    const OneWireBusDevice& device = static_cast<const OneWireBusDevice&>(runtime);
    JsonObject config = output["config"].as<JsonObject>();
    writeOneWireBusDeviceConfigJson(device.config(), config);
    JsonObject runtimeJson = output["runtime"].as<JsonObject>();
    JsonObject scanObject = runtimeJson.createNestedObject("scan");
    scanObject["inProgress"] = device.scan().inProgress;
    scanObject["ready"] = device.scan().ready;
    scanObject["deviceCount"] = device.scan().deviceCount;
    scanObject["truncated"] = device.scan().truncated;
    scanObject["invalidCandidateSeen"] = device.scan().invalidCandidateSeen;
    JsonArray devices = scanObject.createNestedArray("devices");
    for (uint8_t index = 0; index < device.scan().deviceCount; ++index) {
        JsonObject item = devices.createNestedObject();
        char rom[17]{};
        char family[3]{};
        (void)formatOneWireRomAddress(device.scan().devices[index], rom);
        family[0] = "0123456789ABCDEF"[(device.scan().devices[index].bytes[0] >> 4) & 0x0F];
        family[1] = "0123456789ABCDEF"[device.scan().devices[index].bytes[0] & 0x0F];
        family[2] = '\0';
        item["address"] = rom;
        item["familyCode"] = family;
    }
}

} // namespace ewfm
