#include "platform/MqttManager.h"

#include "debug/Debug.h"

namespace ewfm {

#undef SM_CLASS
#define SM_CLASS MqttManager

MqttManager::MqttManager() : StateMachine((PState)&MqttManager::Idle) {
#if defined(ARDUINO) && !defined(UNIT_TEST) && defined(WITH_HOME_ASSISTANT)
    client_.setCallback([this](char* topic, uint8_t* payload, unsigned int length) { dispatchIncoming(topic, payload, length); });
    client_.setBufferSize(kMqttPacketBufferBytes);
#endif
}

void MqttManager::begin(const WifiManager& wifiManager) {
    wifiManager_ = &wifiManager;
    configured_ = true;
}

void MqttManager::applySettings(const MqttSettings& settings, const std::vector<uint8_t>& caCertPem) {
    settings_ = settings;
    caCertPem_ = caCertPem;
    ++settingsRevision_;
}

void MqttManager::setWill(const std::string& topic, const std::string& payload, bool retain) {
    willTopic_ = topic;
    willPayload_ = payload;
    willRetain_ = retain;
}

void MqttManager::onMessage(MessageCallback callback) {
    onMessageCallbacks_.push_back(std::move(callback));
}

void MqttManager::onConnect(ConnectCallback callback) {
    onConnectCallbacks_.push_back(std::move(callback));
}

void MqttManager::onDisconnect(DisconnectCallback callback) {
    onDisconnectCallbacks_.push_back(std::move(callback));
}

bool MqttManager::publish(const std::string& topic, const std::string& payload, bool retain) {
    return publish(topic, reinterpret_cast<const uint8_t*>(payload.data()), payload.size(), retain);
}

bool MqttManager::publish(const std::string& topic, const uint8_t* payload, size_t length, bool retain) {
#if defined(ARDUINO) && !defined(UNIT_TEST) && defined(WITH_HOME_ASSISTANT)
    if (!connected_) {
        return false;
    }
    const bool ok = client_.publish(topic.c_str(), payload, length, retain);
    if (!ok) {
        // PubSubClient::publish() fails silently (oversized packet for the configured buffer,
        // or an underlying write error) - log it once here rather than at every call site, since
        // callers otherwise have no way to notice a dropped publish.
        EWFM_MQTT_LOG_WARN("mqtt publish failed (topic=%s, payloadBytes=%u)", topic.c_str(), static_cast<unsigned>(length));
    }
    return ok;
#else
#if defined(UNIT_TEST)
    if (connected_) {
        publishedMessages_.push_back(PublishedMessage{topic, std::string(reinterpret_cast<const char*>(payload), length), retain});
    }
#else
    (void)topic;
    (void)payload;
    (void)length;
    (void)retain;
#endif
    return connected_;
#endif
}

bool MqttManager::subscribe(const std::string& topic) {
#if defined(ARDUINO) && !defined(UNIT_TEST) && defined(WITH_HOME_ASSISTANT)
    if (!connected_) {
        return false;
    }
    return client_.subscribe(topic.c_str());
#else
#if defined(UNIT_TEST)
    if (connected_) {
        subscribedTopics_.push_back(topic);
    }
#else
    (void)topic;
#endif
    return connected_;
#endif
}

void MqttManager::dispatchIncoming(const char* topic, const uint8_t* payload, unsigned int length) {
    if (topic == nullptr) {
        return;
    }
    const std::string topicStr(topic);
    for (const auto& callback : onMessageCallbacks_) {
        if (callback) {
            callback(topicStr, payload, length);
        }
    }
}

bool MqttManager::mqttReady() const {
    return configured_ && wifiManager_ != nullptr && wifiManager_->stationReady() && settings_.enabled && !settings_.host.empty();
}

SM_STATE(Idle) {
    if (!configured_) {
        return;
    }
    SM_GOTO(WaitingForStation);
}

SM_STATE(WaitingForStation) {
    if (!configured_) {
        SM_GOTO(Idle);
    }

    if (!mqttReady()) {
        if (!stationWaitLogged_) {
            EWFM_MQTT_LOG_INFO("waiting for WiFi station readiness / mqtt settings");
            stationWaitLogged_ = true;
        }
        return;
    }

    stationWaitLogged_ = false;
    // A fresh readiness-driven transition (WiFi just came back, or first boot) is not the
    // "broker keeps refusing us" scenario the retry backoff exists for — attempt immediately.
    everAttempted_ = false;
    SM_GOTO(Connecting);
}

SM_STATE(Connecting) {
    if (!mqttReady()) {
        SM_GOTO(WaitingForStation);
    }

    if (settings_.useTls && caCertPem_.empty()) {
        EWFM_MQTT_LOG_WARN("useTls enabled but no CA certificate configured; refusing to connect");
        SM_GOTO(Faulted);
    }

    if (everAttempted_ && static_cast<uint32_t>(uptime_ - lastConnectAttemptMs_) < kMqttRetryDelayMs) {
        return;
    }
    everAttempted_ = true;
    lastConnectAttemptMs_ = uptime_;

    doConnect();
    if (connected_) {
        appliedRevision_ = settingsRevision_;
        EWFM_MQTT_LOG_INFO("mqtt connected");
        for (const auto& callback : onConnectCallbacks_) {
            if (callback) {
                callback();
            }
        }
        SM_GOTO(Connected);
    }
    EWFM_MQTT_LOG_WARN("mqtt connect attempt failed, retrying");
}

SM_STATE(Connected) {
    if (!mqttReady()) {
        doDisconnect();
        EWFM_MQTT_LOG_INFO("mqtt disconnected: dependency lost");
        for (const auto& callback : onDisconnectCallbacks_) {
            if (callback) {
                callback();
            }
        }
        SM_GOTO(WaitingForStation);
    }

    if (appliedRevision_ != settingsRevision_) {
        doDisconnect();
        // A deliberate settings update (e.g. REST PUT) should reconnect immediately, not wait
        // out the retry backoff meant for throttling repeated failed-connection attempts.
        everAttempted_ = false;
        EWFM_MQTT_LOG_INFO("mqtt settings changed, reconnecting");
        for (const auto& callback : onDisconnectCallbacks_) {
            if (callback) {
                callback();
            }
        }
        SM_GOTO(Connecting);
    }

#if defined(ARDUINO) && !defined(UNIT_TEST) && defined(WITH_HOME_ASSISTANT)
    client_.loop();
    if (!client_.connected()) {
        connected_ = false;
        EWFM_MQTT_LOG_WARN("mqtt connection lost");
        for (const auto& callback : onDisconnectCallbacks_) {
            if (callback) {
                callback();
            }
        }
        SM_GOTO(WaitingForStation);
    }
#endif
}

SM_STATE(Faulted) {
    if (!mqttReady()) {
        SM_GOTO(WaitingForStation);
    }
    if (!(settings_.useTls && caCertPem_.empty())) {
        SM_GOTO(WaitingForStation);
    }
}

void MqttManager::doConnect() {
#if defined(ARDUINO) && !defined(UNIT_TEST) && defined(WITH_HOME_ASSISTANT)
    if (settings_.useTls) {
        secureClient_.setCACert(reinterpret_cast<const char*>(caCertPem_.data()));
        client_.setClient(secureClient_);
    } else {
        client_.setClient(plainClient_);
    }
    client_.setServer(settings_.host.c_str(), settings_.port);

    const char* username = settings_.username.empty() ? nullptr : settings_.username.c_str();
    const char* password = settings_.password.empty() ? nullptr : settings_.password.c_str();

    bool ok;
    if (!willTopic_.empty()) {
        ok = client_.connect(settings_.clientId.c_str(), username, password, willTopic_.c_str(), 0, willRetain_, willPayload_.c_str());
    } else {
        ok = client_.connect(settings_.clientId.c_str(), username, password);
    }
    if (!ok) {
        // PubSubClient::state() distinguishes network-level failures (timeout/refused, negative
        // codes) from broker-level CONNACK rejections (bad credentials/unauthorized/etc, positive
        // codes) - see PubSubClient.h's MQTT_CONNECT_* / MQTT_CONNECTION_* constants.
        EWFM_MQTT_LOG_WARN("mqtt client_.connect() failed: state=%d host=%s port=%u useTls=%d", client_.state(), settings_.host.c_str(),
                           static_cast<unsigned>(settings_.port), static_cast<int>(settings_.useTls));
    }
    connected_ = ok;
#else
    connected_ = true;
#endif
    ++connectCount_;
}

void MqttManager::doDisconnect() {
#if defined(ARDUINO) && !defined(UNIT_TEST) && defined(WITH_HOME_ASSISTANT)
    client_.disconnect();
#endif
    connected_ = false;
    ++disconnectCount_;
}

} // namespace ewfm
