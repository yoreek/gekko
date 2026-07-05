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

    const JsonObjectConst configInput = input["config"].as<JsonObjectConst>();
    if (configInput.isNull()) {
        error = "onewire bus config is required";
        return false;
    }

    OneWireBusDeviceConfigV1 config{};
    if (!config.parseJson(configInput, error)) {
        return false;
    }
    request.name = config.name;
    request.enabled = config.enabled != 0U;
    if (!config.validate().ok()) {
        error = "onewire bus config is invalid";
        return false;
    }
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    const size_t size = oneWireBusDeviceConfigSize(config);
    if (!encodeOneWireBusDeviceConfig(config, buffer, size) || !request.configBlob.assign(buffer, size)) {
        error = "failed to encode onewire bus config";
        return false;
    }
    return true;
}

bool OneWireBusDeviceApiAdapter::parseUpdateConfigRequest(const JsonObjectConst& input, IDeviceRuntime& runtime,
                                                          DeviceConfigUpdateRequest& request, const char*& error) const {
    const JsonObjectConst configInput = input["config"].as<JsonObjectConst>();
    if (configInput.isNull()) {
        error = "onewire bus config is required";
        return false;
    }

    OneWireBusDeviceConfigV1 config = static_cast<const OneWireBusDevice&>(runtime).config();
    if (!config.parseJson(configInput, error)) {
        return false;
    }
    if (!config.validate().ok()) {
        error = "onewire bus config is invalid";
        return false;
    }
    request = {};
    request.configVersion = OneWireBusDevice::descriptor().currentConfigVersion;
    request.enabled = config.enabled != 0U;
    if (!copyBoundedText(request.name, config.name)) {
        error = "device base config is invalid";
        return false;
    }
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
    device.config().writeJson(config);
    JsonObject runtimeJson = output["runtime"].as<JsonObject>();
    JsonObject scanObject = runtimeJson.createNestedObject("scan");
    scanObject["inProgress"] = device.scan().inProgress;
    scanObject["ready"] = device.scan().ready;
    scanObject["deviceCount"] = device.scan().deviceCount;
    scanObject["truncated"] = device.scan().truncated;
    scanObject["invalidCrcSeen"] = device.scan().invalidCandidateSeen;
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
