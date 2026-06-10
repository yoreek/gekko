#include "portal/ws/PortalWebSocketMessages.h"

#include <ArduinoJson.h>

namespace ewfm {

namespace {
void fillDeviceEventPayload(JsonDocument& payload, const DeviceEvent& event) {
    payload["device_id"] = event.deviceId;
    payload["type_id"] = event.typeId;
    payload["registry_revision"] = event.registryRevision;
    payload["config_revision"] = event.configRevision;
    payload["previous_status"] = static_cast<uint8_t>(event.previousStatus);
    payload["status"] = static_cast<uint8_t>(event.status);
    payload["pending_persistence"] = event.pendingPersistence;
    payload["command_accepted"] = event.commandAccepted;
    if (!event.detail.empty()) {
        payload["detail"] = event.detail.c_str();
    }
}
} // namespace

std::string PortalWebSocketMessages::buildEnvelope(const char* topic, const uint32_t revision, JsonDocument& payload) {
    std::string output;
    output.reserve(128U + payload.memoryUsage());
    output += "{\"topic\":\"";
    output += topic;
    output += "\",\"revision\":";
    output += std::to_string(revision);
    output += ",\"payload\":";
    char payloadBuffer[768];
    const size_t payloadLength = serializeJson(payload, payloadBuffer, sizeof(payloadBuffer));
    output.append(payloadBuffer, payloadLength);
    output += '}';
    return output;
}

std::string PortalWebSocketMessages::buildHello(const uint32_t revision, const uint32_t registryRevision, const size_t clientCount) {
    DynamicJsonDocument payload(128);
    payload["state"] = "connected";
    payload["clients"] = clientCount;
    payload["registry_revision"] = registryRevision;
    return buildEnvelope("hello", revision, payload);
}

std::string PortalWebSocketMessages::buildDeviceUpsert(const DeviceEvent& event) {
    DynamicJsonDocument payload(512);
    fillDeviceEventPayload(payload, event);
    return buildEnvelope("device.upsert", event.registryRevision, payload);
}

std::string PortalWebSocketMessages::buildDeviceRemove(const DeviceEvent& event) {
    DynamicJsonDocument payload(256);
    payload["device_id"] = event.deviceId;
    payload["registry_revision"] = event.registryRevision;
    payload["pending_persistence"] = event.pendingPersistence;
    if (!event.detail.empty()) {
        payload["detail"] = event.detail.c_str();
    }
    return buildEnvelope("device.remove", event.registryRevision, payload);
}

std::string PortalWebSocketMessages::buildDeviceCommandResult(const DeviceEvent& event) {
    DynamicJsonDocument payload(512);
    fillDeviceEventPayload(payload, event);
    return buildEnvelope("device.command_result", event.registryRevision, payload);
}

std::string PortalWebSocketMessages::buildWifiStatus(const WifiManager& wifiManager, const IWifiDriver& wifiDriver,
                                                     const uint32_t revision) {
    DynamicJsonDocument payload(384);
    payload["wifi_status"] = wifiManager.connected()          ? "connected"
                             : wifiManager.connecting()       ? "connecting"
                             : wifiManager.apMode()           ? "ap"
                             : wifiManager.bleConfigRunning() ? "ble_config"
                                                              : "idle";
    payload["wifi_interface_up"] = wifiManager.networkStackReady();
    payload["station_ready"] = wifiManager.stationReady();
    payload["setup_ap_ready"] = wifiManager.setupApReady();
    payload["station_ip"] = wifiDriver.stationIp();
    payload["setup_ap_ip"] = wifiDriver.setupApIp();
    payload["retry_count"] = wifiManager.retryCount();
    return buildEnvelope("wifi.status", revision, payload);
}

std::string PortalWebSocketMessages::buildOtaStatus(const bool enabled, const bool hasError, const uint32_t freeSketchSpace,
                                                    const uint32_t revision) {
    DynamicJsonDocument payload(128);
    payload["enabled"] = enabled;
    payload["has_error"] = hasError;
    payload["free_sketch_space"] = freeSketchSpace;
    return buildEnvelope("ota.status", revision, payload);
}

std::string PortalWebSocketMessages::buildSystemStatus(const char* status, const bool rebooting, const uint32_t revision) {
    DynamicJsonDocument payload(128);
    payload["status"] = status;
    payload["rebooting"] = rebooting;
    return buildEnvelope("system.status", revision, payload);
}

} // namespace ewfm
