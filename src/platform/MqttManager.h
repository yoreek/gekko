#pragma once

#include "config/MqttConfig.h"
#include "core/StateMachine.h"
#include "wifi/WifiManager.h"

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#if defined(ARDUINO) && !defined(UNIT_TEST) && defined(WITH_HOME_ASSISTANT)
#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#endif

namespace ewfm {

class MqttManager final : public StateMachine {
public:
    using MessageCallback = std::function<void(const std::string& topic, const uint8_t* payload, size_t length)>;
    using ConnectCallback = std::function<void()>;
    using DisconnectCallback = std::function<void()>;

    MqttManager();

    // Wires the WiFi dependency only; does not start the backend (mirrors ArduinoOtaService::begin).
    void begin(const WifiManager& wifiManager);

    // Can be called repeatedly at runtime (e.g. from a REST settings update). Bumps an internal
    // revision counter so a currently-connected session cleanly reconnects with the new settings.
    void applySettings(const MqttSettings& settings, const std::vector<uint8_t>& caCertPem);

    void setWill(const std::string& topic, const std::string& payload, bool retain);

    // Multi-subscriber: every registered callback fires for every event/message - callers never
    // overwrite each other (HaDiscoveryBridge and SystemHaPublisher both subscribe independently).
    void onMessage(MessageCallback callback);
    void onConnect(ConnectCallback callback);
    void onDisconnect(DisconnectCallback callback);

    bool publish(const std::string& topic, const std::string& payload, bool retain);
    bool subscribe(const std::string& topic);

    bool connected() const {
        return connected_;
    }
    bool waitingForStation() const {
        return is((PState)&MqttManager::WaitingForStation);
    }
#if defined(UNIT_TEST)
    uint16_t connectCount() const {
        return connectCount_;
    }
    uint16_t disconnectCount() const {
        return disconnectCount_;
    }
    struct PublishedMessage {
        std::string topic;
        std::string payload;
        bool retain{false};
    };
    const std::vector<PublishedMessage>& publishedMessages() const {
        return publishedMessages_;
    }
    void clearPublishedMessages() {
        publishedMessages_.clear();
    }
    const std::vector<std::string>& subscribedTopics() const {
        return subscribedTopics_;
    }
    void simulateIncomingMessage(const std::string& topic, const std::string& payload) {
        dispatchIncoming(topic.c_str(), reinterpret_cast<const uint8_t*>(payload.data()), static_cast<unsigned int>(payload.size()));
    }
#endif

private:
    void Idle();
    void WaitingForStation();
    void Connecting();
    void Connected();
    void Faulted();

    bool mqttReady() const;
    void doConnect();
    void doDisconnect();
    void dispatchIncoming(const char* topic, const uint8_t* payload, unsigned int length);

    static constexpr uint32_t kMqttRetryDelayMs = 3000;

    const WifiManager* wifiManager_{nullptr};
    MqttSettings settings_{};
    std::vector<uint8_t> caCertPem_{};
    uint32_t settingsRevision_{0};
    uint32_t appliedRevision_{0};
    bool configured_{false};
    bool connected_{false};
    bool stationWaitLogged_{false};
    bool everAttempted_{false};
    uint32_t lastConnectAttemptMs_{0};
    std::string willTopic_{};
    std::string willPayload_{};
    bool willRetain_{false};
    std::vector<MessageCallback> onMessageCallbacks_{};
    std::vector<ConnectCallback> onConnectCallbacks_{};
    std::vector<DisconnectCallback> onDisconnectCallbacks_{};
    uint16_t connectCount_{0};
    uint16_t disconnectCount_{0};
#if defined(UNIT_TEST)
    std::vector<PublishedMessage> publishedMessages_{};
    std::vector<std::string> subscribedTopics_{};
#endif
#if defined(ARDUINO) && !defined(UNIT_TEST) && defined(WITH_HOME_ASSISTANT)
    WiFiClient plainClient_;
    WiFiClientSecure secureClient_;
    PubSubClient client_{plainClient_};
#endif
};

} // namespace ewfm
