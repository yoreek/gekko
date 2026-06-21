#include "portal/ws/PortalWebSocketMessages.h"

#include <ArduinoJson.h>
#include <vector>

namespace ewfm {

namespace {
const char* deviceStatusToString(const DeviceStatus status) {
    switch (status) {
    case DeviceStatus::Creating:
        return "creating";
    case DeviceStatus::Starting:
        return "starting";
    case DeviceStatus::Ready:
        return "ready";
    case DeviceStatus::Disabled:
        return "disabled";
    case DeviceStatus::Faulted:
        return "faulted";
    case DeviceStatus::DependencyBlocked:
        return "dependency_blocked";
    case DeviceStatus::Reconfiguring:
        return "reconfiguring";
    case DeviceStatus::Stopping:
        return "stopping";
    case DeviceStatus::Deleting:
        return "deleting";
    case DeviceStatus::Unknown:
    default:
        return "unknown";
    }
}

void fillDeviceEventPayload(JsonDocument& payload, const DeviceEvent& event) {
    if (!event.eventKind.empty()) {
        payload["event_kind"] = JsonString(event.eventKind.c_str(), JsonString::Copied);
    }
    payload["device_id"] = event.deviceId;
    payload["type_id"] = event.typeId;
    if (!event.name.empty()) {
        payload["name"] = JsonString(event.name.c_str(), JsonString::Copied);
    }
    if (!event.typeName.empty()) {
        payload["type"] = JsonString(event.typeName.c_str(), JsonString::Copied);
    }
    payload["registry_revision"] = event.registryRevision;
    payload["config_revision"] = event.configRevision;
    payload["previous_status"] = static_cast<uint8_t>(event.previousStatus);
    payload["status"] = static_cast<uint8_t>(event.status);
    payload["pending_persistence"] = event.pendingPersistence;
    payload["command_accepted"] = event.commandAccepted;
    if (!event.detail.empty()) {
        payload["detail"] = JsonString(event.detail.c_str(), JsonString::Copied);
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
    const size_t payloadSize = measureJson(payload) + 1U;
    std::vector<char> payloadBuffer(payloadSize);
    const size_t payloadLength = serializeJson(payload, payloadBuffer.data(), payloadBuffer.size());
    output.append(payloadBuffer.data(), payloadLength);
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

void PortalWebSocketMessages::fillDeviceRuntimePayload(JsonDocument& payload, const IDeviceRuntime& runtime,
                                                       const DeviceStatus effectiveStatus, const uint32_t revision,
                                                       const bool pendingPersistence, const IDeviceApiAdapter* adapter,
                                                       const char* eventKind) {
    JsonObject output = payload.to<JsonObject>();
    if (adapter != nullptr) {
        adapter->writeDeviceJson(runtime, output);
    } else {
        output["device_id"] = runtime.deviceId();
        output["type_id"] = runtime.typeId();
        output["config_version"] = runtime.configVersion();
        output["config_revision"] = runtime.configRevision();
        output["enabled"] = runtime.enabled();
        output["name"] = JsonString(runtime.name(), JsonString::Copied);
        output["status"] = deviceStatusToString(runtime.status());
    }

    if (eventKind != nullptr) {
        output["event_kind"] = eventKind;
    }
    const DeviceStatus lifecycleStatus = runtime.status();
    output["device_id"] = runtime.deviceId();
    output["type_id"] = runtime.typeId();
    output["config_version"] = runtime.configVersion();
    output["config_revision"] = runtime.configRevision();
    if (output["deps"].isNull()) {
        JsonArray deps = output.createNestedArray("deps");
        const DeviceDependencyLink* dependencyLinks = runtime.dependencyLinks();
        const uint8_t dependencyCount = runtime.dependencyCount();
        for (uint8_t index = 0; index < dependencyCount && dependencyLinks != nullptr; ++index) {
            JsonObject item = deps.createNestedObject();
            item["role"] = deviceDependencyRoleName(dependencyLinks[index].role);
            item["device_id"] = dependencyLinks[index].deviceId;
        }
        output["has_deps"] = dependencyCount > 0;
    }
    output["status"] = deviceStatusToString(runtime.status());
    output["lifecycle_status"] = deviceStatusToString(lifecycleStatus);
    output["effective_status"] = deviceStatusToString(effectiveStatus);
    output["registry_revision"] = revision;
    output["pending_persistence"] = pendingPersistence;
}

std::string PortalWebSocketMessages::buildDeviceUpsert(const IDeviceRuntime& runtime, const DeviceStatus effectiveStatus,
                                                       const uint32_t revision, const bool pendingPersistence,
                                                       const IDeviceApiAdapter* adapter, const char* eventKind) {
    DynamicJsonDocument payload(1536);
    fillDeviceRuntimePayload(payload, runtime, effectiveStatus, revision, pendingPersistence, adapter, eventKind);
    return buildEnvelope("device.upsert", revision, payload);
}

std::string PortalWebSocketMessages::buildDeviceCommandResult(const IDeviceRuntime& runtime, const DeviceStatus effectiveStatus,
                                                              const uint32_t revision, const bool pendingPersistence,
                                                              const IDeviceApiAdapter* adapter, const char* eventKind) {
    DynamicJsonDocument payload(1536);
    fillDeviceRuntimePayload(payload, runtime, effectiveStatus, revision, pendingPersistence, adapter, eventKind);
    return buildEnvelope("device.command_result", revision, payload);
}

std::string PortalWebSocketMessages::buildDeviceUpsert(const DeviceEvent& event) {
    DynamicJsonDocument payload(512);
    fillDeviceEventPayload(payload, event);
    return buildEnvelope("device.upsert", event.registryRevision, payload);
}

std::string PortalWebSocketMessages::buildDeviceRemove(const DeviceEvent& event) {
    DynamicJsonDocument payload(384);
    if (!event.eventKind.empty()) {
        payload["event_kind"] = JsonString(event.eventKind.c_str(), JsonString::Copied);
    }
    payload["device_id"] = event.deviceId;
    payload["type_id"] = event.typeId;
    payload["registry_revision"] = event.registryRevision;
    payload["pending_persistence"] = event.pendingPersistence;
    if (!event.name.empty()) {
        payload["name"] = JsonString(event.name.c_str(), JsonString::Copied);
    }
    if (!event.typeName.empty()) {
        payload["type"] = JsonString(event.typeName.c_str(), JsonString::Copied);
    }
    if (!event.detail.empty()) {
        payload["detail"] = JsonString(event.detail.c_str(), JsonString::Copied);
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
    const std::string stationIp = wifiDriver.stationIp();
    const std::string setupApIp = wifiDriver.setupApIp();
    payload["wifi_status"] = wifiManager.connected()          ? "connected"
                             : wifiManager.connecting()       ? "connecting"
                             : wifiManager.apMode()           ? "ap"
                             : wifiManager.bleConfigRunning() ? "ble_config"
                                                              : "idle";
    payload["wifi_interface_up"] = wifiManager.networkStackReady();
    payload["station_ready"] = wifiManager.stationReady();
    payload["setup_ap_ready"] = wifiManager.setupApReady();
    payload["station_ip"] = JsonString(stationIp.c_str(), JsonString::Copied);
    payload["setup_ap_ip"] = JsonString(setupApIp.c_str(), JsonString::Copied);
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
