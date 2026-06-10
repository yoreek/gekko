#include "portal/ws/PortalWebSocketManager.h"

#if defined(ARDUINO) && !defined(UNIT_TEST)
#include <ESPAsyncWebServer.h>
#include <Update.h>
#endif

namespace ewfm {

PortalWebSocketManager::PortalWebSocketManager(DeviceEventDispatcher* dispatcher) : dispatcher_(dispatcher) {}

PortalWebSocketManager::~PortalWebSocketManager() = default;

void PortalWebSocketManager::attachDispatcher() {
    if (dispatcher_ != nullptr) {
        dispatcher_->registerSink(*this);
    }
}

void PortalWebSocketManager::detachDispatcher() {
    if (dispatcher_ != nullptr) {
        dispatcher_->unregisterSink(*this);
    }
}

#if defined(ARDUINO) && !defined(UNIT_TEST)
bool PortalWebSocketManager::begin(AsyncWebServer& server) {
    if (socket_ != nullptr) {
        return true;
    }

    socket_ = new AsyncWebSocket("/ws");
    if (socket_ == nullptr) {
        return false;
    }

    socket_->onEvent([this](AsyncWebSocket* ws, AsyncWebSocketClient* client, const AwsEventType type, void*, uint8_t*, size_t) {
        switch (type) {
        case WS_EVT_CONNECT:
            (void)ws;
            if (client != nullptr) {
                const std::string hello = PortalWebSocketMessages::buildHello(lastRevision_, lastRevision_, socket_->count());
                client->text(hello.c_str(), hello.length());
            }
            break;
        case WS_EVT_DISCONNECT:
        case WS_EVT_ERROR:
        case WS_EVT_PONG:
        case WS_EVT_DATA:
        case WS_EVT_PING:
        default:
            break;
        }
    });

    server.addHandler(socket_);
    attachDispatcher();
    return true;
}

void PortalWebSocketManager::end(AsyncWebServer& server) {
    detachDispatcher();
    if (socket_ != nullptr) {
        socket_->closeAll();
        server.removeHandler(socket_);
        delete socket_;
        socket_ = nullptr;
    }
}
#endif

void PortalWebSocketManager::tick(uint32_t now, const WifiManager& wifiManager, const IWifiDriver& wifiDriver) {
    (void)now;
    const std::string wifiPayload = PortalWebSocketMessages::buildWifiStatus(wifiManager, wifiDriver, lastRevision_);
    if (wifiPayload != lastWifiStatusPayload_) {
        lastWifiStatusPayload_ = wifiPayload;
        sendText(wifiPayload);
    }
}

void PortalWebSocketManager::onDeviceEvent(const DeviceEvent& event) {
    lastRevision_ = event.registryRevision;

    switch (event.kind) {
    case DeviceEventKind::DeviceDeleted:
        sendText(PortalWebSocketMessages::buildDeviceRemove(event));
        return;
    case DeviceEventKind::CommandAccepted:
    case DeviceEventKind::CommandRejected:
        sendText(PortalWebSocketMessages::buildDeviceCommandResult(event));
        return;
    case DeviceEventKind::DeviceCreated:
    case DeviceEventKind::DeviceUpdated:
    case DeviceEventKind::StatusChanged:
    case DeviceEventKind::StateChanged:
    case DeviceEventKind::ConfigPersisted:
    case DeviceEventKind::RetainedStateChanged:
    case DeviceEventKind::PersistencePendingCleared:
        sendText(PortalWebSocketMessages::buildDeviceUpsert(event));
        return;
    case DeviceEventKind::RegistryLoaded:
    default:
        sendText(PortalWebSocketMessages::buildHello(event.registryRevision, event.registryRevision, 0));
        return;
    }
}

void PortalWebSocketManager::tickFastLoop(uint32_t now) {
    (void)now;
}

void PortalWebSocketManager::tick100ms(uint32_t now) {
    (void)now;
}

void PortalWebSocketManager::tick1s(uint32_t now) {
    (void)now;
}

void PortalWebSocketManager::sendText(const std::string& payload) {
    if (payload.empty()) {
        return;
    }

#if defined(ARDUINO) && !defined(UNIT_TEST)
    if (socket_ == nullptr || socket_->count() == 0U) {
        return;
    }
    socket_->textAll(payload.c_str(), payload.length());
#else
    sentMessages_.push_back(payload);
#endif
}

void PortalWebSocketManager::broadcastHello() {
    sendText(PortalWebSocketMessages::buildHello(lastRevision_, lastRevision_, 0));
}

#if defined(UNIT_TEST)
const std::vector<std::string>& PortalWebSocketManager::sentMessages() const {
    return sentMessages_;
}

size_t PortalWebSocketManager::sentMessageCount() const {
    return sentMessages_.size();
}
#endif

} // namespace ewfm
