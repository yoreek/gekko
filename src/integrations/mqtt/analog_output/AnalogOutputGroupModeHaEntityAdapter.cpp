#include "integrations/mqtt/analog_output/AnalogOutputGroupModeHaEntityAdapter.h"

#include "devices/analog/composer/AnalogOutputComposerDevice.h"
#include "devices/registry/DeviceRegistry.h"
#include "integrations/mqtt/HaDiscoveryConstants.h"
#include "integrations/mqtt/HaDiscoveryEnvelope.h"

namespace ewfm {

namespace {
constexpr const char* kModeChannel = "analog_output_group_mode";
}

const AnalogOutputGroupModeHaEntityAdapter& AnalogOutputGroupModeHaEntityAdapter::instance() {
    static const AnalogOutputGroupModeHaEntityAdapter adapter;
    return adapter;
}

DeviceTypeId AnalogOutputGroupModeHaEntityAdapter::typeId() const {
    return AnalogOutputComposerDevice::descriptor().typeId;
}

const char* AnalogOutputGroupModeHaEntityAdapter::typeName() const {
    return "analog_output_group_mode";
}

const char* AnalogOutputGroupModeHaEntityAdapter::haComponent() const {
    return "select";
}

void AnalogOutputGroupModeHaEntityAdapter::buildDiscoveryPayload(const IDeviceRuntime& runtime, const std::string& uniqueId,
                                                                 const std::string& effectiveName, const HaTopicBuilder& topicFor,
                                                                 JsonObject output) const {
    (void)runtime;
    writeHaEntityIdentity(output, uniqueId, effectiveName + " Mode");
    output[ha::key::kStateTopic] = topicFor(kModeChannel, ha::topic::kState);
    output[ha::key::kCommandTopic] = topicFor(kModeChannel, ha::topic::kSet);
    JsonArray options = output.createNestedArray(ha::key::kOptions);
    options.add("off");
    options.add("manual");
    options.add("scheduled");
    output[ha::key::kEntityCategory] = "config";
    output[ha::key::kIcon] = "mdi:calendar-sync";
}

void AnalogOutputGroupModeHaEntityAdapter::publishState(const IDeviceRuntime& runtime, const HaTopicBuilder& topicFor,
                                                        const HaStatePublisher& publish) const {
    const IAnalogOutputGroupRuntime* group = runtime.analogOutputGroupRuntime();
    if (group != nullptr) {
        publish(topicFor(kModeChannel, ha::topic::kState), analogOutputModeName(group->analogOutputGroupMode()));
    }
}

bool AnalogOutputGroupModeHaEntityAdapter::applyCommand(DeviceRegistry& registry, const IDeviceRuntime& runtime, const DeviceId deviceId,
                                                        const std::string& commandKey, const std::string& payload,
                                                        const uint32_t now) const {
    (void)runtime;
    if (commandKey != kModeChannel || (payload != "off" && payload != "manual" && payload != "scheduled")) {
        return false;
    }
    return registry.command(DeviceCommand{DeviceCommandType::Custom, deviceId, payload}, now).ok();
}

} // namespace ewfm
