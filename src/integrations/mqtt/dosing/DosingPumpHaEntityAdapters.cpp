#include "integrations/mqtt/dosing/DosingPumpHaEntityAdapters.h"

#include "devices/dosing/DosingPumpDevice.h"
#include "devices/registry/DeviceRegistry.h"
#include "integrations/mqtt/HaDiscoveryConstants.h"
#include "integrations/mqtt/HaDiscoveryEnvelope.h"

#include <cctype>
#include <cstdio>

namespace ewfm {

namespace {
void formatCentiMl(uint32_t centiMl, char* buffer, size_t size) {
    std::snprintf(buffer, size, "%.2f", static_cast<double>(centiMl) / 100.0);
}
} // namespace

DosingPumpHaEntityAdapter::DosingPumpHaEntityAdapter(DosingPumpHaEntityAdapterConfig config) : config_(config) {}

DeviceTypeId DosingPumpHaEntityAdapter::typeId() const {
    return DosingPumpDevice::descriptor().typeId;
}

const char* DosingPumpHaEntityAdapter::typeName() const {
    return config_.typeName;
}

const char* DosingPumpHaEntityAdapter::haComponent() const {
    return config_.haComponent;
}

void DosingPumpHaEntityAdapter::buildDiscoveryPayload(const IDeviceRuntime& runtime, const std::string& uniqueId,
                                                      const std::string& effectiveName, const HaTopicBuilder& topicFor,
                                                      JsonObject output) const {
    (void)runtime;
    // Suffixed per entity - otherwise all five entities of one pump show the same friendly name.
    writeHaEntityIdentity(output, uniqueId, effectiveName + " " + config_.nameSuffix);
    output[ha::key::kStateTopic] = topicFor(config_.channel, ha::topic::kState);
    output[ha::key::kIcon] = config_.icon;

    switch (config_.kind) {
    case DosingPumpHaEntityKind::RunState: {
        // Enum sensors must not carry unit_of_measurement/state_class - HA rejects the payload.
        output[ha::key::kDeviceClass] = "enum";
        JsonArray options = output.createNestedArray(ha::key::kOptions);
        options.add("idle");
        options.add("dosing");
        break;
    }
    case DosingPumpHaEntityKind::TodayDosed:
        output[ha::key::kDeviceClass] = "volume";
        output[ha::key::kUnitOfMeasurement] = "mL";
        // total_increasing: HA treats the drop at local midnight as a meter reset, which is
        // exactly how todayDosedCentiMl behaves.
        output[ha::key::kStateClass] = "total_increasing";
        break;
    case DosingPumpHaEntityKind::ContainerLevel:
        output[ha::key::kDeviceClass] = "volume";
        output[ha::key::kUnitOfMeasurement] = "mL";
        output[ha::key::kStateClass] = "measurement";
        break;
    case DosingPumpHaEntityKind::ContainerEmpty:
        output[ha::key::kDeviceClass] = "problem"; // ON = container empty = problem
        output[ha::key::kPayloadOn] = ha::payload::kOn;
        output[ha::key::kPayloadOff] = ha::payload::kOff;
        break;
    case DosingPumpHaEntityKind::AutoMode:
        output[ha::key::kCommandTopic] = topicFor(config_.channel, ha::topic::kSet);
        output[ha::key::kPayloadOn] = ha::payload::kOn;
        output[ha::key::kPayloadOff] = ha::payload::kOff;
        output[ha::key::kStateOn] = ha::payload::kOn;
        output[ha::key::kStateOff] = ha::payload::kOff;
        output[ha::key::kEntityCategory] = "config";
        break;
    }
}

void DosingPumpHaEntityAdapter::publishState(const IDeviceRuntime& runtime, const HaTopicBuilder& topicFor,
                                             const HaStatePublisher& publish) const {
    // DosingPumpDevice exposes no capability interface for this read surface; downcast is safe
    // because the bridge routes by runtime->typeId() (thermostat idiom).
    const auto& pump = static_cast<const DosingPumpDevice&>(runtime);
    char buffer[16];
    switch (config_.kind) {
    case DosingPumpHaEntityKind::RunState:
        publish(topicFor(config_.channel, ha::topic::kState), pump.running() ? "dosing" : "idle");
        break;
    case DosingPumpHaEntityKind::TodayDosed:
        formatCentiMl(pump.todayDosedCentiMl(), buffer, sizeof(buffer));
        publish(topicFor(config_.channel, ha::topic::kState), buffer);
        break;
    case DosingPumpHaEntityKind::ContainerLevel:
        formatCentiMl(pump.containerCurrentCentiMl(), buffer, sizeof(buffer));
        publish(topicFor(config_.channel, ha::topic::kState), buffer);
        break;
    case DosingPumpHaEntityKind::ContainerEmpty:
        // Meaningful even without a level sensor (falls back to the tracked volume hitting zero).
        publish(topicFor(config_.channel, ha::topic::kState), pump.containerEmpty() ? ha::payload::kOn : ha::payload::kOff);
        break;
    case DosingPumpHaEntityKind::AutoMode:
        publish(topicFor(config_.channel, ha::topic::kState), pump.autoMode() ? ha::payload::kOn : ha::payload::kOff);
        break;
    }
}

bool DosingPumpHaEntityAdapter::applyCommand(DeviceRegistry& registry, const IDeviceRuntime& runtime, DeviceId deviceId,
                                             const std::string& commandKey, const std::string& payload, uint32_t now) const {
    (void)runtime;
    (void)now;
    if (config_.kind != DosingPumpHaEntityKind::AutoMode || commandKey != config_.channel) {
        return false;
    }
    std::string normalized;
    normalized.reserve(payload.size());
    for (char c : payload) {
        normalized.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }
    // Same Custom-command grammar the REST setMode command uses (DeviceRegistryController::cmd).
    DeviceCommand command;
    if (normalized == "on") {
        command = DeviceCommand{DeviceCommandType::Custom, deviceId, "auto"};
    } else if (normalized == "off") {
        command = DeviceCommand{DeviceCommandType::Custom, deviceId, "manual"};
    } else {
        return false;
    }
    return registry.command(command, 0).ok();
}

} // namespace ewfm
