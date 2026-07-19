#include "integrations/rest/onewire_bus/OneWireBusDeviceApiAdapter.h"

namespace ewfm {

void OneWireBusDeviceApiAdapter::writeRuntimeJson(const OneWireBusDevice& device, JsonObject runtimeJson) const {
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
