#pragma once

#include "devices/core/DeviceTypes.h"
#include "integrations/common/DeviceApiAdapter.h"
#include "wifi/WifiDriver.h"
#include "wifi/WifiManager.h"

#include <ArduinoJson.h>
#include <string>

namespace ewfm {

class PortalWebSocketMessages {
public:
    static std::string buildHello(uint32_t revision, uint32_t registryRevision, size_t clientCount);
    static std::string buildDeviceUpsert(const IDeviceRuntime& runtime, DeviceStatus effectiveStatus, uint32_t revision,
                                         bool pendingPersistence, const IDeviceApiAdapter* adapter, const char* eventKind);
    static std::string buildDeviceCommandResult(const IDeviceRuntime& runtime, DeviceStatus effectiveStatus, uint32_t revision,
                                                bool pendingPersistence, const IDeviceApiAdapter* adapter, const char* eventKind);
    static std::string buildDeviceRemove(const DeviceEvent& event);
    static std::string buildWifiStatus(const WifiManager& wifiManager, const IWifiDriver& wifiDriver, uint32_t revision);
    static std::string buildOtaStatus(bool enabled, bool hasError, uint32_t freeSketchSpace, uint32_t revision);
    static std::string buildSystemStatus(const char* status, bool rebooting, uint32_t revision);
    static std::string buildMqttStatus(bool enabled, bool connected, bool waitingForStation, uint32_t revision);

private:
    static std::string buildEnvelope(const char* topic, uint32_t revision, JsonDocument& payload);
    static void fillDeviceRuntimePayload(JsonDocument& payload, const IDeviceRuntime& runtime, DeviceStatus effectiveStatus,
                                         uint32_t revision, bool pendingPersistence, const IDeviceApiAdapter* adapter,
                                         const char* eventKind);
};

} // namespace ewfm
