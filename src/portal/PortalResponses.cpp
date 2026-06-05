#include "portal/PortalResponses.h"

#include <ArduinoJson.h>

namespace ewfm {

std::string wifiScanResponseJson(const std::vector<WifiNetwork>& networks) {
    DynamicJsonDocument doc(2048);
    JsonArray array = doc.createNestedArray("networks");

    for (const auto& network : networks) {
        JsonObject item = array.createNestedObject();
        item["ssid"] = network.ssid;
        item["rssi"] = network.rssi;
        item["channel"] = network.channel;
    }

    std::string payload;
    serializeJson(doc, payload);
    return payload;
}

std::string otaStatusResponseJson(size_t freeSketchSpace, bool hasError) {
    DynamicJsonDocument doc(256);
    doc["enabled"] = true;
    doc["free_sketch_space"] = freeSketchSpace;
    doc["has_error"] = hasError;

    std::string payload;
    serializeJson(doc, payload);
    return payload;
}

} // namespace ewfm
