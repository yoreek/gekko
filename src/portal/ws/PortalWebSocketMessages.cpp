#include "portal/ws/PortalWebSocketMessages.h"

#include <ArduinoJson.h>
#include <cstring>

namespace ewfm {

std::string PortalWebSocketMessages::buildEnvelope(const char* topic, const uint32_t revision, const JsonDocument& payload) {
    std::string output;
    output.reserve(40U + std::strlen(topic) + measureJson(payload));
    output += "{\"topic\":\"";
    output += topic;
    output += "\",\"revision\":";
    output += std::to_string(revision);
    output += ",\"payload\":";
    // ArduinoJson 6 appends to the string (v7 would replace it - revisit on upgrade).
    serializeJson(payload, output);
    output += '}';
    return output;
}

std::string PortalWebSocketMessages::buildHello(const uint32_t revision, const uint32_t registryRevision, const size_t clientCount) {
    StaticJsonDocument<128> payload;
    payload["state"] = "connected";
    payload["clients"] = clientCount;
    payload["registryRevision"] = registryRevision;
    return buildEnvelope("hello", revision, payload);
}

void PortalWebSocketMessages::fillDeviceRuntimePayload(JsonDocument& payload, const IDeviceRuntime& runtime,
                                                       const DeviceStatus effectiveStatus, const uint32_t revision,
                                                       const bool pendingPersistence, const IDeviceApiAdapter* adapter,
                                                       const char* eventKind) {
    JsonObject output = payload.to<JsonObject>();
    if (adapter != nullptr) {
        adapter->writeDeviceJson(runtime, effectiveStatus, output);
    } else {
        IDeviceApiAdapter::writeFallbackDeviceJson(runtime, effectiveStatus, "unknown", output);
    }

    if (eventKind != nullptr) {
        output["eventKind"] = eventKind;
    }
    (void)revision;
    (void)pendingPersistence;
}

std::string PortalWebSocketMessages::buildDeviceUpsert(const IDeviceRuntime& runtime, const DeviceStatus effectiveStatus,
                                                       const uint32_t revision, const bool pendingPersistence,
                                                       const IDeviceApiAdapter* adapter, const char* eventKind) {
    StaticJsonDocument<2048> payload;
    fillDeviceRuntimePayload(payload, runtime, effectiveStatus, revision, pendingPersistence, adapter, eventKind);
    return buildEnvelope("device.upsert", revision, payload);
}

std::string PortalWebSocketMessages::buildDeviceCommandResult(const IDeviceRuntime& runtime, const DeviceStatus effectiveStatus,
                                                              const uint32_t revision, const bool pendingPersistence,
                                                              const IDeviceApiAdapter* adapter, const char* eventKind) {
    StaticJsonDocument<2048> payload;
    fillDeviceRuntimePayload(payload, runtime, effectiveStatus, revision, pendingPersistence, adapter, eventKind);
    return buildEnvelope("device.command_result", revision, payload);
}

std::string PortalWebSocketMessages::buildDeviceRemove(const DeviceEvent& event) {
    StaticJsonDocument<384> payload;
    if (!event.eventKind.empty()) {
        payload["eventKind"] = JsonString(event.eventKind.c_str(), JsonString::Copied);
    }
    payload["deviceId"] = event.deviceId;
    payload["typeId"] = event.typeId;
    payload["registryRevision"] = event.registryRevision;
    if (!event.name.empty()) {
        payload["name"] = JsonString(event.name.c_str(), JsonString::Copied);
    }
    if (!event.typeName.empty()) {
        payload["typeName"] = JsonString(event.typeName.c_str(), JsonString::Copied);
    }
    if (!event.detail.empty()) {
        payload["detail"] = JsonString(event.detail.c_str(), JsonString::Copied);
    }
    return buildEnvelope("device.remove", event.registryRevision, payload);
}

std::string PortalWebSocketMessages::buildWifiStatus(const WifiManager& wifiManager, const IWifiDriver& wifiDriver,
                                                     const uint32_t revision) {
    StaticJsonDocument<384> payload;
    const std::string stationIp = wifiDriver.stationIp();
    const std::string setupApIp = wifiDriver.setupApIp();
    payload["wifiStatus"] = wifiManager.connected()          ? "connected"
                            : wifiManager.connecting()       ? "connecting"
                            : wifiManager.apMode()           ? "ap"
                            : wifiManager.bleConfigRunning() ? "ble_config"
                                                             : "idle";
    payload["wifiInterfaceUp"] = wifiManager.networkStackReady();
    payload["stationReady"] = wifiManager.stationReady();
    payload["setupApReady"] = wifiManager.setupApReady();
    payload["stationIp"] = JsonString(stationIp.c_str(), JsonString::Copied);
    payload["setupApIp"] = JsonString(setupApIp.c_str(), JsonString::Copied);
    payload["retryCount"] = wifiManager.retryCount();
    return buildEnvelope("wifi.status", revision, payload);
}

std::string PortalWebSocketMessages::buildOtaStatus(const bool enabled, const bool hasError, const uint32_t freeSketchSpace,
                                                    const uint32_t revision) {
    StaticJsonDocument<128> payload;
    payload["enabled"] = enabled;
    payload["hasError"] = hasError;
    payload["freeSketchSpace"] = freeSketchSpace;
    return buildEnvelope("ota.status", revision, payload);
}

std::string PortalWebSocketMessages::buildSystemStatus(const char* status, const bool rebooting, const uint32_t revision) {
    StaticJsonDocument<128> payload;
    payload["status"] = status;
    payload["rebooting"] = rebooting;
    return buildEnvelope("system.status", revision, payload);
}

std::string PortalWebSocketMessages::buildMqttStatus(const bool enabled, const bool connected, const bool waitingForStation,
                                                     const uint32_t revision) {
    StaticJsonDocument<128> payload;
    payload["enabled"] = enabled;
    payload["connected"] = connected;
    payload["waitingForStation"] = waitingForStation;
    return buildEnvelope("mqtt.status", revision, payload);
}

std::string PortalWebSocketMessages::buildTimeStatus(const bool synced, const uint32_t currentEpochUtc, const char* timezoneId,
                                                     const char* source, const uint32_t revision) {
    StaticJsonDocument<160> payload;
    payload["synced"] = synced;
    payload["currentEpochUtc"] = currentEpochUtc;
    payload["timezoneId"] = timezoneId;
    payload["source"] = source;
    return buildEnvelope("time.status", revision, payload);
}

} // namespace ewfm
