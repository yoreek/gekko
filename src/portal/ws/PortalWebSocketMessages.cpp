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
        payload["eventKind"] = JsonString(event.eventKind.c_str(), JsonString::Copied);
    } else {
        payload["eventKind"] = deviceEventKindName(event.kind);
    }
    payload["deviceId"] = event.deviceId;
    payload["typeId"] = event.typeId;
    if (!event.name.empty()) {
        payload["name"] = JsonString(event.name.c_str(), JsonString::Copied);
    }
    if (!event.typeName.empty()) {
        payload["typeName"] = JsonString(event.typeName.c_str(), JsonString::Copied);
    }
    payload["registryRevision"] = event.registryRevision;
    payload["configRevision"] = event.configRevision;
    payload["previousStatus"] = static_cast<uint8_t>(event.previousStatus);
    payload["status"] = static_cast<uint8_t>(event.status);
    payload["pendingPersistence"] = event.pendingPersistence;
    payload["commandAccepted"] = event.commandAccepted;
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
        JsonObject record = output.createNestedObject("record");
        record["id"] = runtime.deviceId();
        record["typeName"] = "unknown";
        record["configRevision"] = runtime.configRevision();

        JsonObject config = output.createNestedObject("config");
        config["name"] = JsonString(runtime.name() != nullptr ? runtime.name() : "", JsonString::Copied);
        config["enabled"] = runtime.enabled();
        JsonArray deps = config.createNestedArray("deps");
        const DeviceDependencyLink* dependencyLinks = runtime.dependencyLinks();
        const uint8_t dependencyCount = runtime.dependencyCount();
        for (uint8_t index = 0; index < dependencyCount && dependencyLinks != nullptr; ++index) {
            JsonObject item = deps.createNestedObject();
            item["role"] = deviceDependencyRoleName(dependencyLinks[index].role);
            item["deviceId"] = dependencyLinks[index].deviceId;
        }

        JsonObject runtimeJson = output.createNestedObject("runtime");
        runtimeJson["status"] = deviceStatusToString(runtime.status());
        runtimeJson["effectiveStatus"] = deviceStatusToString(effectiveStatus);
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
    DynamicJsonDocument payload(128);
    payload["enabled"] = enabled;
    payload["hasError"] = hasError;
    payload["freeSketchSpace"] = freeSketchSpace;
    return buildEnvelope("ota.status", revision, payload);
}

std::string PortalWebSocketMessages::buildSystemStatus(const char* status, const bool rebooting, const uint32_t revision) {
    DynamicJsonDocument payload(128);
    payload["status"] = status;
    payload["rebooting"] = rebooting;
    return buildEnvelope("system.status", revision, payload);
}

} // namespace ewfm
