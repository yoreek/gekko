#include "portal/PortalResponses.h"

#if defined(ARDUINO) && !defined(UNIT_TEST)
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#endif

namespace ewfm {

#if defined(ARDUINO) && !defined(UNIT_TEST)
void writeWifiScanResponseJson(::AsyncResponseStream& out, const std::vector<WifiNetwork>& networks) {
    StaticJsonDocument<2048> doc;
    JsonArray array = doc.createNestedArray("networks");

    for (const auto& network : networks) {
        JsonObject item = array.createNestedObject();
        item["ssid"] = network.ssid;
        item["rssi"] = network.rssi;
        item["channel"] = network.channel;
    }

    serializeJson(doc, out);
}

void writeOtaStatusResponseJson(::AsyncResponseStream& out, size_t freeSketchSpace, bool hasError) {
    StaticJsonDocument<128> doc;
    doc["enabled"] = true;
    doc["freeSketchSpace"] = freeSketchSpace;
    doc["hasError"] = hasError;

    serializeJson(doc, out);
}
#else
void writeWifiScanResponseJson(::AsyncResponseStream& out, const std::vector<WifiNetwork>& networks) {
    (void)out;
    (void)networks;
}

void writeOtaStatusResponseJson(::AsyncResponseStream& out, size_t freeSketchSpace, bool hasError) {
    (void)out;
    (void)freeSketchSpace;
    (void)hasError;
}
#endif

} // namespace ewfm
