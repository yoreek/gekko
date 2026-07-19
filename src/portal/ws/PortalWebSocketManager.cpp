#include "portal/ws/PortalWebSocketManager.h"

#include "time/NtpManager.h"

#if defined(ARDUINO) && !defined(UNIT_TEST)
#include <ESPAsyncWebServer.h>
#include <Update.h>
#endif

namespace ewfm {

PortalWebSocketManager::PortalWebSocketManager(DeviceEventDispatcher* dispatcher, DeviceRegistry* deviceRegistry)
    : dispatcher_(dispatcher), deviceRegistry_(deviceRegistry) {}

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
                connectedClientCount_ = socket_ != nullptr ? socket_->count() : connectedClientCount_;
                const std::string hello = PortalWebSocketMessages::buildHello(lastRevision_, lastRevision_, socket_->count());
                client->text(hello.c_str(), hello.length());
                resyncSnapshots();
                publishDeviceSnapshots();
            }
            break;
        case WS_EVT_DISCONNECT:
            connectedClientCount_ = socket_ != nullptr ? socket_->count() : 0U;
            break;
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
    tick(now, wifiManager, wifiDriver, false, false, 0U, false, false, false, nullptr);
}

void PortalWebSocketManager::tick(uint32_t now, const WifiManager& wifiManager, const IWifiDriver& wifiDriver, const bool otaEnabled,
                                  const bool otaHasError, const uint32_t freeSketchSpace) {
    tick(now, wifiManager, wifiDriver, otaEnabled, otaHasError, freeSketchSpace, false, false, false, nullptr);
}

void PortalWebSocketManager::tick(uint32_t now, const WifiManager& wifiManager, const IWifiDriver& wifiDriver, const bool otaEnabled,
                                  const bool otaHasError, const uint32_t freeSketchSpace, const bool mqttEnabled, const bool mqttConnected,
                                  const bool mqttWaitingForStation) {
    tick(now, wifiManager, wifiDriver, otaEnabled, otaHasError, freeSketchSpace, mqttEnabled, mqttConnected, mqttWaitingForStation,
         nullptr);
}

void PortalWebSocketManager::tick(uint32_t now, const WifiManager& wifiManager, const IWifiDriver& wifiDriver, const bool otaEnabled,
                                  const bool otaHasError, const uint32_t freeSketchSpace, const bool mqttEnabled, const bool mqttConnected,
                                  const bool mqttWaitingForStation, const NtpManager* ntpManager) {
    (void)now;
    const std::string wifiPayload = PortalWebSocketMessages::buildWifiStatus(wifiManager, wifiDriver, lastRevision_);
    const std::string otaPayload = PortalWebSocketMessages::buildOtaStatus(otaEnabled, otaHasError, freeSketchSpace, lastRevision_);
    const std::string mqttPayload =
        PortalWebSocketMessages::buildMqttStatus(mqttEnabled, mqttConnected, mqttWaitingForStation, lastRevision_);
    const bool timeSynced = ntpManager != nullptr && ntpManager->synced();
    // Reports the epoch at the moment of the last successful sync (not a live "now") - the
    // frontend advances it client-side using its own timer, avoiding a TimeLib dependency in this
    // file (which, unlike NtpManager itself, must stay buildable/testable under env:native).
    uint32_t currentEpochUtc = 0U;
    if (ntpManager != nullptr && ntpManager->lastSyncedUtc().has_value()) {
        currentEpochUtc = ntpManager->lastSyncedUtc()->unixtime();
    }
    const std::string timezoneId = ntpManager != nullptr ? ntpManager->config().timezoneId : std::string();
    const char* timeSource = timeSourceName(ntpManager != nullptr ? ntpManager->lastSource() : TimeSource::None);
    const std::string timePayload =
        PortalWebSocketMessages::buildTimeStatus(timeSynced, currentEpochUtc, timezoneId.c_str(), timeSource, lastRevision_);
    publishSnapshotPayloads(wifiPayload, otaPayload, mqttPayload, timePayload);
}

void PortalWebSocketManager::onDeviceEvent(const DeviceEvent& event) {
    lastRevision_ = event.registryRevision;

    switch (event.kind) {
    case DeviceEventKind::DeviceDeleted:
        sendText(PortalWebSocketMessages::buildDeviceRemove(event));
        return;
    case DeviceEventKind::CommandAccepted:
    case DeviceEventKind::CommandRejected: {
        if (deviceRegistry_ == nullptr) {
            return;
        }
        const IDeviceRuntime* runtime = deviceRegistry_->runtime(event.deviceId);
        if (runtime == nullptr) {
            // The device was deleted before this queued event was dispatched. device.command_result
            // is contractually required to carry the full device payload, and there's no live
            // runtime left to build one from - drop the stale event rather than send a malformed one.
            return;
        }
        const IDeviceApiAdapter* adapter = adapters_.find(runtime->typeId());
        sendText(PortalWebSocketMessages::buildDeviceCommandResult(
            *runtime, deviceRegistry_->effectiveStatus(event.deviceId), deviceRegistry_->registryRevision(),
            deviceRegistry_->hasPendingPersistence(), adapter, deviceEventKindName(event.kind)));
        return;
    }
    case DeviceEventKind::PersistencePendingCleared:
        return;
    case DeviceEventKind::DeviceCreated:
    case DeviceEventKind::DeviceUpdated:
    case DeviceEventKind::StatusChanged:
    case DeviceEventKind::StateChanged:
    case DeviceEventKind::ConfigPersisted:
    case DeviceEventKind::RetainedStateChanged: {
        if (deviceRegistry_ == nullptr) {
            return;
        }
        const IDeviceRuntime* runtime = deviceRegistry_->runtime(event.deviceId);
        if (runtime == nullptr) {
            // Same reasoning as above: device.upsert must always carry the full payload, so a
            // stale event for an already-deleted device must be dropped, not sent malformed.
            return;
        }
        const IDeviceApiAdapter* adapter = adapters_.find(runtime->typeId());
        sendText(PortalWebSocketMessages::buildDeviceUpsert(*runtime, deviceRegistry_->effectiveStatus(event.deviceId),
                                                            deviceRegistry_->registryRevision(), deviceRegistry_->hasPendingPersistence(),
                                                            adapter, deviceEventKindName(event.kind)));
        return;
    }
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
    connectedClientCount_ = socket_ != nullptr ? socket_->count() : 0U;
    if (connectedClientCount_ == 0U) {
        return;
    }
    socket_->textAll(payload.c_str(), payload.length());
#else
    if (connectedClientCount_ == 0U) {
        return;
    }
    sentMessages_.push_back(payload);
#endif
}

void PortalWebSocketManager::broadcastHello() {
    sendText(PortalWebSocketMessages::buildHello(lastRevision_, lastRevision_, 0));
}

void PortalWebSocketManager::resyncSnapshots() {
    resyncSnapshots_ = true;
}

void PortalWebSocketManager::publishDeviceSnapshots() {
    if (deviceRegistry_ == nullptr) {
        return;
    }

    deviceRegistry_->forEachRuntime([this](const IDeviceRuntime& runtime) {
        const IDeviceApiAdapter* adapter = adapters_.find(runtime.typeId());
        sendText(PortalWebSocketMessages::buildDeviceUpsert(runtime, deviceRegistry_->effectiveStatus(runtime.deviceId()),
                                                            deviceRegistry_->registryRevision(), deviceRegistry_->hasPendingPersistence(),
                                                            adapter, "snapshot"));
    });
}

void PortalWebSocketManager::publishSnapshotPayloads(const std::string& wifiPayload, const std::string& otaPayload,
                                                     const std::string& mqttPayload, const std::string& timePayload) {
    if (resyncSnapshots_ || wifiPayload != lastWifiStatusPayload_) {
        lastWifiStatusPayload_ = wifiPayload;
        sendText(wifiPayload);
    }

    if (resyncSnapshots_ || otaPayload != lastOtaStatusPayload_) {
        lastOtaStatusPayload_ = otaPayload;
        sendText(otaPayload);
    }

    if (resyncSnapshots_ || mqttPayload != lastMqttStatusPayload_) {
        lastMqttStatusPayload_ = mqttPayload;
        sendText(mqttPayload);
    }

    if (resyncSnapshots_ || timePayload != lastTimeStatusPayload_) {
        lastTimeStatusPayload_ = timePayload;
        sendText(timePayload);
    }

    resyncSnapshots_ = false;
}

#if defined(UNIT_TEST)
const std::vector<std::string>& PortalWebSocketManager::sentMessages() const {
    return sentMessages_;
}

size_t PortalWebSocketManager::sentMessageCount() const {
    return sentMessages_.size();
}

void PortalWebSocketManager::setClientCountForTest(const size_t clientCount) {
    connectedClientCount_ = clientCount;
    resyncSnapshots_ = clientCount > 0U;
}

void PortalWebSocketManager::publishDeviceSnapshotsForTest() {
    publishDeviceSnapshots();
}

void PortalWebSocketManager::publishSnapshotPayloadsForTest(const std::string& wifiPayload, const std::string& otaPayload,
                                                            const std::string& mqttPayload, const std::string& timePayload) {
    publishSnapshotPayloads(wifiPayload, otaPayload, mqttPayload, timePayload);
}
#endif

} // namespace ewfm
