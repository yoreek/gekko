#include "integrations/mqtt/analog_output/ScheduledAnalogOutputModeHaEntityAdapter.h"

#include "devices/analog/scheduled/ScheduledAnalogOutputDevice.h"
#include "devices/registry/DeviceRegistry.h"
#include "integrations/mqtt/HaDiscoveryConstants.h"
#include "integrations/mqtt/HaDiscoveryEnvelope.h"

namespace ewfm {

namespace {
constexpr const char* kModeChannel = "analog_output_mode";
}

const ScheduledAnalogOutputModeHaEntityAdapter& ScheduledAnalogOutputModeHaEntityAdapter::instance() {
    static const ScheduledAnalogOutputModeHaEntityAdapter adapter;
    return adapter;
}

DeviceTypeId ScheduledAnalogOutputModeHaEntityAdapter::typeId() const {
    return ScheduledAnalogOutputDevice::descriptor().typeId;
}

const char* ScheduledAnalogOutputModeHaEntityAdapter::typeName() const {
    return "scheduled_analog_output_mode";
}

const char* ScheduledAnalogOutputModeHaEntityAdapter::haComponent() const {
    return "select";
}

void ScheduledAnalogOutputModeHaEntityAdapter::buildDiscoveryPayload(const IDeviceRuntime& runtime, const std::string& uniqueId,
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

void ScheduledAnalogOutputModeHaEntityAdapter::publishState(const IDeviceRuntime& runtime, const HaTopicBuilder& topicFor,
                                                            const HaStatePublisher& publish) const {
    const IScheduledAnalogOutputRuntime* scheduled = runtime.scheduledAnalogOutputRuntime();
    if (scheduled != nullptr) {
        publish(topicFor(kModeChannel, ha::topic::kState), analogOutputModeName(scheduled->analogOutputMode()));
    }
}

bool ScheduledAnalogOutputModeHaEntityAdapter::applyCommand(DeviceRegistry& registry, const IDeviceRuntime& runtime,
                                                            const DeviceId deviceId, const std::string& commandKey,
                                                            const std::string& payload, const uint32_t now) const {
    (void)runtime;
    if (commandKey != kModeChannel || (payload != "off" && payload != "manual" && payload != "scheduled")) {
        return false;
    }
    return registry.command(DeviceCommand{DeviceCommandType::Custom, deviceId, payload}, now).ok();
}

} // namespace ewfm
