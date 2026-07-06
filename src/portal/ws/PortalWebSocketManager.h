#pragma once

#include "devices/registry/DeviceRegistry.h"
#include "integrations/common/DeviceApiAdapter.h"
#include "integrations/common/DeviceEventBus.h"
#include "integrations/common/DeviceEventDispatcher.h"
#include "portal/ws/PortalWebSocketMessages.h"

#include <string>
#include <vector>

#if defined(ARDUINO) && !defined(UNIT_TEST)
class AsyncWebServer;
class AsyncWebSocket;
#endif

namespace ewfm {

class PortalWebSocketManager final : public IDeviceEventSink {
public:
    explicit PortalWebSocketManager(DeviceEventDispatcher* dispatcher = nullptr, DeviceRegistry* deviceRegistry = nullptr);
    ~PortalWebSocketManager() override;

    void attachDispatcher();
    void detachDispatcher();

#if defined(ARDUINO) && !defined(UNIT_TEST)
    bool begin(AsyncWebServer& server);
    void end(AsyncWebServer& server);
#endif

    void tick(uint32_t now, const WifiManager& wifiManager, const IWifiDriver& wifiDriver);
    void tick(uint32_t now, const WifiManager& wifiManager, const IWifiDriver& wifiDriver, bool otaEnabled, bool otaHasError,
              uint32_t freeSketchSpace);
    void tick(uint32_t now, const WifiManager& wifiManager, const IWifiDriver& wifiDriver, bool otaEnabled, bool otaHasError,
              uint32_t freeSketchSpace, bool mqttEnabled, bool mqttConnected, bool mqttWaitingForStation);

    void onDeviceEvent(const DeviceEvent& event) override;
    void tickFastLoop(uint32_t now) override;
    void tick100ms(uint32_t now) override;
    void tick1s(uint32_t now) override;

#if defined(UNIT_TEST)
    const std::vector<std::string>& sentMessages() const;
    size_t sentMessageCount() const;
    void setClientCountForTest(size_t clientCount);
    void publishDeviceSnapshotsForTest();
    void publishSnapshotPayloadsForTest(const std::string& wifiPayload, const std::string& otaPayload,
                                        const std::string& mqttPayload = std::string());
#endif

private:
    void sendText(const std::string& payload);
    void broadcastHello();
    void resyncSnapshots();
    void publishDeviceSnapshots();
    void publishSnapshotPayloads(const std::string& wifiPayload, const std::string& otaPayload, const std::string& mqttPayload);

    DeviceEventDispatcher* dispatcher_{nullptr};
    DeviceRegistry* deviceRegistry_{nullptr};
    DeviceApiAdapterRegistry adapters_{DeviceApiAdapterRegistry::withDefaults()};
    uint32_t lastRevision_{0};
    std::string lastWifiStatusPayload_{};
    std::string lastOtaStatusPayload_{};
    std::string lastMqttStatusPayload_{};
    size_t connectedClientCount_{0};
    bool resyncSnapshots_{true};

#if defined(ARDUINO) && !defined(UNIT_TEST)
    AsyncWebSocket* socket_{nullptr};
#endif

#if defined(UNIT_TEST)
    std::vector<std::string> sentMessages_{};
#endif
};

} // namespace ewfm
