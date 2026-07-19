#include "integrations/mqtt/switch/SwitchOutputHaEntityAdapter.h"

#include "devices/registry/DeviceRegistry.h"
#include "integrations/mqtt/HaDiscoveryConstants.h"
#include "integrations/mqtt/HaDiscoveryEnvelope.h"

#include <cctype>

namespace ewfm {

namespace {
bool equalsIgnoreCase(const std::string& value, const char* expected) {
    size_t index = 0;
    while (index < value.size() && expected[index] != '\0') {
        if (std::tolower(static_cast<unsigned char>(value[index])) != std::tolower(static_cast<unsigned char>(expected[index]))) {
            return false;
        }
        ++index;
    }
    return index == value.size() && expected[index] == '\0';
}
} // namespace

SwitchOutputHaEntityAdapter::SwitchOutputHaEntityAdapter(SwitchOutputHaEntityAdapterConfig config) : config_(config) {}

DeviceTypeId SwitchOutputHaEntityAdapter::typeId() const {
    return config_.typeId;
}

const char* SwitchOutputHaEntityAdapter::typeName() const {
    return config_.typeName;
}

const char* SwitchOutputHaEntityAdapter::haComponent() const {
    return ha::component::kSwitch;
}

void SwitchOutputHaEntityAdapter::buildDiscoveryPayload(const IDeviceRuntime& runtime, const std::string& uniqueId,
                                                        const std::string& effectiveName, const HaTopicBuilder& topicFor,
                                                        JsonObject output) const {
    (void)runtime;
    writeHaEntityIdentity(output, uniqueId, effectiveName);
    output[ha::key::kStateTopic] = topicFor(ha::component::kSwitch, ha::topic::kState);
    output[ha::key::kCommandTopic] = topicFor(ha::component::kSwitch, ha::topic::kSet);
    output[ha::key::kPayloadOn] = ha::payload::kOn;
    output[ha::key::kPayloadOff] = ha::payload::kOff;
    output[ha::key::kStateOn] = ha::payload::kOn;
    output[ha::key::kStateOff] = ha::payload::kOff;
    output[ha::key::kIcon] = config_.icon;
}

void SwitchOutputHaEntityAdapter::publishState(const IDeviceRuntime& runtime, const HaTopicBuilder& topicFor,
                                               const HaStatePublisher& publish) const {
    const ISwitchOutputRuntime* switchRuntime = runtime.switchOutputRuntime();
    if (switchRuntime == nullptr) {
        return;
    }

    const char* payload = switchRuntime->currentOutputState() ? ha::payload::kOn : ha::payload::kOff;
    publish(topicFor(ha::component::kSwitch, ha::topic::kState), payload);
}

bool SwitchOutputHaEntityAdapter::applyCommand(DeviceRegistry& registry, const IDeviceRuntime& runtime, DeviceId deviceId,
                                               const std::string& commandKey, const std::string& payload, uint32_t now) const {
    (void)runtime;
    (void)now;
    if (commandKey != ha::component::kSwitch) {
        return false;
    }

    if (equalsIgnoreCase(payload, "on")) {
        return registry.command(DeviceCommand{DeviceCommandType::SetOutput, deviceId, "true"}, 0).ok();
    }
    if (equalsIgnoreCase(payload, "off")) {
        return registry.command(DeviceCommand{DeviceCommandType::SetOutput, deviceId, "false"}, 0).ok();
    }
    return false;
}

} // namespace ewfm
