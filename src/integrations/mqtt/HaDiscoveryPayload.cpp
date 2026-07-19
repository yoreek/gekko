#include "integrations/mqtt/HaDiscoveryPayload.h"

#include "platform/MqttManager.h"

namespace ewfm {

bool publishHaDiscoveryPayload(MqttManager& mqttManager, const std::string& topic, const JsonDocument& document, size_t& serializedLength) {
    char buffer[kMaxHaDiscoveryPayloadBytes];

    serializedLength = measureJson(document);
    if (serializedLength >= sizeof(buffer)) {
        return false;
    }
    serializedLength = serializeJson(document, buffer, sizeof(buffer));
    return mqttManager.publish(topic, reinterpret_cast<const uint8_t*>(buffer), serializedLength, true);
}

} // namespace ewfm
