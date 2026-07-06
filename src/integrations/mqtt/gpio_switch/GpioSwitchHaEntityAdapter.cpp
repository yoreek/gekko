#include "integrations/mqtt/gpio_switch/GpioSwitchHaEntityAdapter.h"

#include "devices/registry/DeviceRegistry.h"
#include "devices/switch/OutputState.h"
#include "devices/switch/gpio/GpioSwitchDevice.h"

#include <cctype>

namespace ewfm {

const GpioSwitchHaEntityAdapter& GpioSwitchHaEntityAdapter::instance() {
    static const GpioSwitchHaEntityAdapter adapter;
    return adapter;
}

DeviceTypeId GpioSwitchHaEntityAdapter::typeId() const {
    return GpioSwitchDevice::descriptor().typeId;
}

const char* GpioSwitchHaEntityAdapter::typeName() const {
    return "gpio_switch";
}

const char* GpioSwitchHaEntityAdapter::haComponent() const {
    return "switch";
}

void GpioSwitchHaEntityAdapter::buildDiscoveryPayload(const IDeviceRuntime& runtime, const std::string& uniqueId,
                                                      const std::string& effectiveName, const HaTopicBuilder& topicFor,
                                                      JsonObject output) const {
    (void)runtime;
    output["unique_id"] = uniqueId;
    output["object_id"] = uniqueId;
    output["name"] = effectiveName;
    output["state_topic"] = topicFor("switch", "state");
    output["command_topic"] = topicFor("switch", "set");
    output["payload_on"] = "ON";
    output["payload_off"] = "OFF";
    output["state_on"] = "ON";
    output["state_off"] = "OFF";
    output["icon"] = "mdi:toggle-switch";
}

void GpioSwitchHaEntityAdapter::publishState(const IDeviceRuntime& runtime, const HaTopicBuilder& topicFor,
                                             const HaStatePublisher& publish) const {
    const ISwitchOutputRuntime* switchRuntime = runtime.switchOutputRuntime();
    if (switchRuntime == nullptr) {
        return;
    }
    std::string payload;
    switch (switchRuntime->currentOutputState()) {
    case OutputState::On:
        payload = "ON";
        break;
    case OutputState::Off:
        payload = "OFF";
        break;
    case OutputState::Disabled:
    default:
        // HA's binary switch component has no third state; skip the publish rather than invent one.
        return;
    }
    publish(topicFor("switch", "state"), payload);
}

bool GpioSwitchHaEntityAdapter::applyCommand(DeviceRegistry& registry, const IDeviceRuntime& runtime, DeviceId deviceId,
                                             const std::string& commandKey, const std::string& payload, uint32_t now) const {
    (void)runtime;
    (void)now;
    if (commandKey != "switch") {
        return false;
    }
    std::string normalized;
    normalized.reserve(payload.size());
    for (char c : payload) {
        normalized.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }
    DeviceCommand command;
    if (normalized == "on") {
        command = DeviceCommand{DeviceCommandType::SetOutput, deviceId, "on"};
    } else if (normalized == "off") {
        command = DeviceCommand{DeviceCommandType::SetOutput, deviceId, "off"};
    } else {
        return false;
    }
    return registry.command(command, 0).ok();
}

} // namespace ewfm
