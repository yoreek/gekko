#include "config/json/WifiRuntimeConfigJson.h"

namespace ewfm {

void WifiRuntimeConfigJson::write(JsonDocument& doc, const WifiRuntimeConfig& config) {
    JsonObject runtime = doc.createNestedObject("wifi_runtime");
    runtime["max_connect_retries"] = config.maxConnectRetries;
    runtime["connect_timeout_ms"] = config.connectTimeoutMs;
    runtime["retry_delay_ms"] = config.retryDelayMs;
    runtime["setup_ap_enabled"] = config.setupApEnabled;
}

void WifiRuntimeConfigJson::read(JsonDocument& doc, DeviceConfig& config) {
    if (!doc["wifi_runtime"].is<JsonObject>()) {
        return;
    }

    JsonObject runtime = doc["wifi_runtime"];
    config.wifiRuntime.maxConnectRetries = runtime["max_connect_retries"] | config.wifiRuntime.maxConnectRetries;
    config.wifiRuntime.connectTimeoutMs = runtime["connect_timeout_ms"] | config.wifiRuntime.connectTimeoutMs;
    config.wifiRuntime.retryDelayMs = runtime["retry_delay_ms"] | config.wifiRuntime.retryDelayMs;
    config.wifiRuntime.setupApEnabled = runtime["setup_ap_enabled"] | runtime["fallback_ap_enabled"] | config.wifiRuntime.setupApEnabled;
}

} // namespace ewfm
