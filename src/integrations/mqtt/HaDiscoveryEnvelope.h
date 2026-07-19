#pragma once

#include <ArduinoJson.h>
#include <string>

namespace ewfm {

void writeHaEntityIdentity(JsonObject output, const std::string& uniqueId, const std::string& effectiveName);

// Fills the fields every HA MQTT discovery payload shares regardless of entity type: the
// availability topic, has_entity_name, the shared `device` block (grouping every entity - per-device
// or system-level - under one Home Assistant device for this board), and the required `origin` block.
void writeHaDiscoveryEnvelope(JsonObject output, const std::string& nodeId, const std::string& nodeName,
                              const std::string& availabilityTopic);

} // namespace ewfm
