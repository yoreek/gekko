#include "integrations/rest/i2c_bus/I2cBusDeviceApiAdapter.h"

#include <cstdio>

namespace ewfm {

void I2cBusDeviceApiAdapter::writeRuntimeJson(const I2cBusDevice& device, JsonObject runtimeJson) const {
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
