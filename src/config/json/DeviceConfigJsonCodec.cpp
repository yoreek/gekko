#include "config/json/DeviceConfigJsonCodec.h"

#include "config/json/FirmwareUpdateConfigJson.h"
#include "config/json/ProvisioningConfigJson.h"
#include "config/json/WifiConfigJson.h"
#include "config/json/WifiRuntimeConfigJson.h"

namespace ewfm {

void DeviceConfigJsonCodec::write(JsonDocument& doc, const DeviceConfig& config) {
    doc["schema_version"] = config.schemaVersion;
    doc["device_name"] = config.deviceName;
    doc["max_json_bytes"] = config.maxJsonBytes;

    WifiConfigJson::write(doc, config.wifi);
    ProvisioningConfigJson::write(doc, config.provisioning);
    WifiRuntimeConfigJson::write(doc, config.wifiRuntime);
    FirmwareUpdateConfigJson::write(doc, config.firmwareUpdate);
}

void DeviceConfigJsonCodec::read(JsonDocument& doc, DeviceConfig& config) {
    config.schemaVersion = doc["schema_version"] | config.schemaVersion;
    config.deviceName = doc["device_name"] | config.deviceName;
    config.maxJsonBytes = doc["max_json_bytes"] | config.maxJsonBytes;

    WifiConfigJson::read(doc, config);
    ProvisioningConfigJson::read(doc, config);
    WifiRuntimeConfigJson::read(doc, config);
    FirmwareUpdateConfigJson::read(doc, config);
}

} // namespace ewfm
