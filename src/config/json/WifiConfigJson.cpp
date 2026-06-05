#include "config/json/WifiConfigJson.h"

namespace ewfm {

void WifiConfigJson::write(JsonDocument& doc, const WiFiCredentials& credentials) {
    JsonObject wifi = doc.createNestedObject("wifi");
    wifi["ssid"] = credentials.ssid;
    wifi["password"] = nullptr;
    wifi["password_redacted"] = !credentials.password.empty();
}

void WifiConfigJson::read(JsonDocument& doc, DeviceConfig& config) {
    if (!doc["wifi"].is<JsonObject>()) {
        return;
    }

    JsonObject wifi = doc["wifi"];
    config.wifi.ssid = wifi["ssid"] | config.wifi.ssid;
    if (wifi["password"].is<const char*>()) {
        config.wifi.password = wifi["password"].as<std::string>();
    }
}

} // namespace ewfm
