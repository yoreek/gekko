#include "integrations/mqtt/analog_output/AnalogOutputHaEntityAdapter.h"

#include "devices/registry/DeviceRegistry.h"
#include "integrations/mqtt/HaDiscoveryConstants.h"
#include "integrations/mqtt/HaDiscoveryEnvelope.h"

#include <ArduinoJson.h>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <string_view>

namespace ewfm {

namespace {
bool equalsIgnoreCase(std::string_view value, const char* expected) {
    size_t index = 0;
    while (index < value.size() && expected[index] != '\0') {
        if (std::tolower(static_cast<unsigned char>(value[index])) != std::tolower(static_cast<unsigned char>(expected[index]))) {
            return false;
        }
        ++index;
    }
    return index == value.size() && expected[index] == '\0';
}

std::string_view trim(std::string_view value) {
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front())) != 0) {
        value.remove_prefix(1);
    }
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back())) != 0) {
        value.remove_suffix(1);
    }
    return value;
}

bool parseUnsigned(std::string_view text, uint32_t& value) {
    text = trim(text);
    if (text.empty()) {
        return false;
    }
    char buffer[32]{};
    if (text.size() >= sizeof(buffer)) {
        return false;
    }
    std::memcpy(buffer, text.data(), text.size());
    char* end = nullptr;
    const unsigned long parsed = std::strtoul(buffer, &end, 10);
    if (end == buffer || *end != '\0') {
        return false;
    }
    value = static_cast<uint32_t>(parsed);
    return true;
}

bool parsePercent(std::string_view text, uint16_t& level) {
    text = trim(text);
    if (text.empty()) {
        return false;
    }
    if (text.back() != '%') {
        return false;
    }
    uint32_t percent = 0;
    if (!parseUnsigned(text.substr(0, text.size() - 1), percent) || percent > 100U) {
        return false;
    }
    level = static_cast<uint16_t>((static_cast<uint32_t>(percent) * kAnalogOutputLevelMax + 50U) / 100U);
    return true;
}

bool parseStatePayload(const std::string& payload, uint16_t& stateValue) {
    const std::string_view trimmed = trim(payload);
    if (trimmed.empty()) {
        return false;
    }
    if (equalsIgnoreCase(trimmed, "on")) {
        stateValue = kAnalogOutputLevelMax;
        return true;
    }
    if (equalsIgnoreCase(trimmed, "off")) {
        stateValue = 0U;
        return true;
    }
    if (trimmed.front() == '{' && trimmed.back() == '}') {
        StaticJsonDocument<256> doc;
        const DeserializationError error = deserializeJson(doc, payload);
        if (error) {
            return false;
        }

        const JsonVariantConst state = doc["state"];
        if (!state.isNull()) {
            if (state.is<const char*>() && equalsIgnoreCase(state.as<const char*>(), "off")) {
                stateValue = 0U;
                return true;
            }
            if (state.is<const char*>() && equalsIgnoreCase(state.as<const char*>(), "on")) {
                stateValue = kAnalogOutputLevelMax;
            }
        }

        const JsonVariantConst statePercent = doc["statePercent"];
        if (!statePercent.isNull()) {
            const uint32_t percent = statePercent.as<uint32_t>();
            if (percent > 100U) {
                return false;
            }
            stateValue = static_cast<uint16_t>((percent * kAnalogOutputLevelMax + 50U) / 100U);
            return true;
        }

        const JsonVariantConst brightness = doc["brightness"];
        if (!brightness.isNull()) {
            const uint32_t brightnessValue = brightness.as<uint32_t>();
            if (brightnessValue > 255U) {
                return false;
            }
            stateValue = static_cast<uint16_t>((brightnessValue * kAnalogOutputLevelMax + 127U) / 255U);
            return true;
        }

        if (!state.isNull() && !state.is<const char*>()) {
            const uint32_t raw = state.as<uint32_t>();
            if (raw > kAnalogOutputLevelMax) {
                return false;
            }
            stateValue = static_cast<uint16_t>(raw);
            return true;
        }

        return state.isNull() ? false : true;
    }

    uint32_t raw = 0;
    if (parsePercent(trimmed, stateValue)) {
        return true;
    }
    if (!parseUnsigned(trimmed, raw) || raw > kAnalogOutputLevelMax) {
        return false;
    }
    stateValue = static_cast<uint16_t>(raw);
    return true;
}

uint8_t analogOutputLevelToBrightness(uint16_t level) {
    return static_cast<uint8_t>((static_cast<uint32_t>(level) * 255U + (kAnalogOutputLevelMax / 2U)) / kAnalogOutputLevelMax);
}

uint8_t analogOutputLevelToPercent(uint16_t level) {
    return static_cast<uint8_t>((static_cast<uint32_t>(level) * 100U + (kAnalogOutputLevelMax / 2U)) / kAnalogOutputLevelMax);
}
} // namespace

AnalogOutputHaEntityAdapter::AnalogOutputHaEntityAdapter(AnalogOutputHaEntityAdapterConfig config) : config_(config) {}

DeviceTypeId AnalogOutputHaEntityAdapter::typeId() const {
    return config_.typeId;
}

const char* AnalogOutputHaEntityAdapter::typeName() const {
    return config_.typeName;
}

const char* AnalogOutputHaEntityAdapter::haComponent() const {
    return ha::component::kLight;
}

void AnalogOutputHaEntityAdapter::buildDiscoveryPayload(const IDeviceRuntime& runtime, const std::string& uniqueId,
                                                        const std::string& effectiveName, const HaTopicBuilder& topicFor,
                                                        JsonObject output) const {
    (void)runtime;
    writeHaEntityIdentity(output, uniqueId, effectiveName);
    output[ha::key::kStateTopic] = topicFor(ha::component::kLight, ha::topic::kState);
    output[ha::key::kCommandTopic] = topicFor(ha::component::kLight, ha::topic::kSet);
    output[ha::key::kPayloadOn] = ha::payload::kOn;
    output[ha::key::kPayloadOff] = ha::payload::kOff;
    output["brightness_state_topic"] = topicFor("light_brightness", ha::topic::kState);
    output["brightness_command_topic"] = topicFor("light_brightness", ha::topic::kSet);
    output["brightness_scale"] = 255;
    output["on_command_type"] = "brightness";
    output["optimistic"] = false;
    output[ha::key::kIcon] = config_.icon;
}

void AnalogOutputHaEntityAdapter::publishState(const IDeviceRuntime& runtime, const HaTopicBuilder& topicFor,
                                               const HaStatePublisher& publish) const {
    const IAnalogOutputRuntime* analogRuntime = runtime.analogOutputRuntime();
    if (analogRuntime == nullptr) {
        return;
    }
    const uint16_t level = analogRuntime->currentOutputState();
    const char* state = level == 0U ? ha::payload::kOff : ha::payload::kOn;
    const std::string brightness = std::to_string(analogOutputLevelToBrightness(level));
    publish(topicFor(ha::component::kLight, ha::topic::kState), state);
    publish(topicFor("light_brightness", ha::topic::kState), brightness);
}

bool AnalogOutputHaEntityAdapter::applyCommand(DeviceRegistry& registry, const IDeviceRuntime& runtime, DeviceId deviceId,
                                               const std::string& commandKey, const std::string& payload, uint32_t now) const {
    (void)runtime;
    if (commandKey == ha::component::kLight) {
        if (equalsIgnoreCase(payload, "off")) {
            return registry.command(DeviceCommand{DeviceCommandType::SetOutput, deviceId, "0"}, now).ok();
        }
        if (equalsIgnoreCase(payload, "on")) {
            return registry.command(DeviceCommand{DeviceCommandType::SetOutput, deviceId, "100"}, now).ok();
        }
        uint16_t level = 0U;
        if (!parseStatePayload(payload, level)) {
            return false;
        }
        return registry
            .command(DeviceCommand{DeviceCommandType::SetOutput, deviceId, std::to_string(analogOutputLevelToPercent(level))}, now)
            .ok();
    }
    if (commandKey == "light_brightness") {
        uint32_t brightness = 0U;
        if (!parseUnsigned(payload, brightness) || brightness > 255U) {
            return false;
        }
        const uint32_t percent = (brightness * 100U + 127U) / 255U;
        return registry.command(DeviceCommand{DeviceCommandType::SetOutput, deviceId, std::to_string(percent)}, now).ok();
    }
    return false;
}

} // namespace ewfm
