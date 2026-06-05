#include "config/json/FirmwareUpdateConfigJson.h"

namespace ewfm {

void FirmwareUpdateConfigJson::write(JsonDocument& doc, const FirmwareUpdateConfig& config) {
    JsonObject ota = doc.createNestedObject("firmware_update");
    ota["web_ota_enabled"] = config.webOtaEnabled;
    ota["max_metadata_bytes"] = config.maxMetadataBytes;
}

void FirmwareUpdateConfigJson::read(JsonDocument& doc, DeviceConfig& config) {
    if (!doc["firmware_update"].is<JsonObject>()) {
        return;
    }

    JsonObject ota = doc["firmware_update"];
    config.firmwareUpdate.webOtaEnabled = ota["web_ota_enabled"] | config.firmwareUpdate.webOtaEnabled;
    config.firmwareUpdate.maxMetadataBytes = ota["max_metadata_bytes"] | config.firmwareUpdate.maxMetadataBytes;
}

} // namespace ewfm
