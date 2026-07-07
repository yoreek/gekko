#include "integrations/mqtt/HaDiscoveryEnvelope.h"

namespace ewfm {

void writeHaDiscoveryEnvelope(JsonObject output, const std::string& nodeId, const std::string& nodeName,
                              const std::string& availabilityTopic) {
    output["availability_topic"] = availabilityTopic;
    output["has_entity_name"] = true;

    JsonObject device = output.createNestedObject("device");
    JsonArray identifiers = device.createNestedArray("identifiers");
    identifiers.add(nodeId);
    device["name"] = nodeName.empty() ? nodeId : nodeName;
    device["manufacturer"] = "ESP32WIFIManager";

    JsonObject origin = output.createNestedObject("origin");
    origin["name"] = "ESP32WIFIManager";
}

} // namespace ewfm
