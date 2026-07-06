#include "integrations/mqtt/gpio_switch/GpioSwitchHaEntityAdapter.h"

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
                                                      const std::string& effectiveName, const std::string& stateTopic,
                                                      const std::string& commandTopic, JsonObject output) const {
    (void)runtime;
    output["unique_id"] = uniqueId;
    output["object_id"] = uniqueId;
    output["name"] = effectiveName;
    output["state_topic"] = stateTopic;
    output["command_topic"] = commandTopic;
    output["payload_on"] = "ON";
    output["payload_off"] = "OFF";
    output["state_on"] = "ON";
    output["state_off"] = "OFF";
}

bool GpioSwitchHaEntityAdapter::buildStatePayload(const IDeviceRuntime& runtime, std::string& payload) const {
    const ISwitchOutputRuntime* switchRuntime = runtime.switchOutputRuntime();
    if (switchRuntime == nullptr) {
        return false;
    }
    switch (switchRuntime->currentOutputState()) {
    case OutputState::On:
        payload = "ON";
        return true;
    case OutputState::Off:
        payload = "OFF";
        return true;
    case OutputState::Disabled:
    default:
        // HA's binary switch component has no third state; skip the publish rather than invent one.
        return false;
    }
}

bool GpioSwitchHaEntityAdapter::parseCommand(const std::string& payload, DeviceId deviceId, DeviceCommand& command) const {
    std::string normalized;
    normalized.reserve(payload.size());
    for (char c : payload) {
        normalized.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }
    if (normalized == "on") {
        command = DeviceCommand{DeviceCommandType::SetOutput, deviceId, "on"};
        return true;
    }
    if (normalized == "off") {
        command = DeviceCommand{DeviceCommandType::SetOutput, deviceId, "off"};
        return true;
    }
    return false;
}

} // namespace ewfm
