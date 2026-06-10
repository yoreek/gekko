#pragma once

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
    explicit PortalWebSocketManager(DeviceEventDispatcher* dispatcher = nullptr);
    ~PortalWebSocketManager() override;

    void attachDispatcher();
    void detachDispatcher();

#if defined(ARDUINO) && !defined(UNIT_TEST)
    bool begin(AsyncWebServer& server);
    void end(AsyncWebServer& server);
#endif

    void tick(uint32_t now, const WifiManager& wifiManager, const IWifiDriver& wifiDriver);

    void onDeviceEvent(const DeviceEvent& event) override;
    void tickFastLoop(uint32_t now) override;
    void tick100ms(uint32_t now) override;
    void tick1s(uint32_t now) override;

#if defined(UNIT_TEST)
    const std::vector<std::string>& sentMessages() const;
    size_t sentMessageCount() const;
#endif

private:
    void sendText(const std::string& payload);
    void broadcastHello();

    DeviceEventDispatcher* dispatcher_{nullptr};
    uint32_t lastRevision_{0};
    std::string lastWifiStatusPayload_{};

#if defined(ARDUINO) && !defined(UNIT_TEST)
    AsyncWebSocket* socket_{nullptr};
#endif

#if defined(UNIT_TEST)
    std::vector<std::string> sentMessages_{};
#endif
};

} // namespace ewfm
