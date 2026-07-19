#pragma once

#include <ArduinoJson.h>
#include <cstddef>
#include <string>

namespace ewfm {

class MqttManager;

inline constexpr size_t kMaxHaDiscoveryPayloadBytes = 1536;

bool publishHaDiscoveryPayload(MqttManager& mqttManager, const std::string& topic, const JsonDocument& document, size_t& serializedLength);

} // namespace ewfm
