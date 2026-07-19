#include "integrations/mqtt/HaDiscoveryEnvelope.h"

#include "integrations/mqtt/HaDiscoveryConstants.h"

namespace ewfm {

void writeHaEntityIdentity(JsonObject output, const std::string& uniqueId, const std::string& effectiveName) {
    output[ha::key::kUniqueId] = uniqueId;
    output[ha::key::kObjectId] = uniqueId;
    output[ha::key::kName] = effectiveName;
}

void writeHaDiscoveryEnvelope(JsonObject output, const std::string& nodeId, const std::string& nodeName,
                              const std::string& availabilityTopic) {
    output[ha::key::kBaseTopic] = nodeId;
    output[ha::key::kAvailabilityTopic] = availabilityTopic;
    output[ha::key::kHasEntityName] = true;

    JsonObject device = output.createNestedObject(ha::key::kDevice);
    JsonArray identifiers = device.createNestedArray(ha::key::kIdentifiers);
    identifiers.add(nodeId);
    device[ha::key::kName] = nodeName.empty() ? nodeId : nodeName;
    device[ha::key::kManufacturer] = "Gekko";

    JsonObject origin = output.createNestedObject(ha::key::kOrigin);
    origin[ha::key::kName] = "Gekko";
}

} // namespace ewfm
