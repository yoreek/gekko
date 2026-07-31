#include "integrations/mqtt/pixel_strip/PixelEffectSolidHaEntityAdapter.h"

#include "devices/pixel/effects/PixelEffectSolidDevice.h"
#include "devices/registry/DeviceRegistry.h"
#include "integrations/mqtt/HaDiscoveryConstants.h"
#include "integrations/mqtt/HaDiscoveryEnvelope.h"

#include <cctype>
#include <cstdlib>
#include <cstring>
#include <string>
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

bool parseChannel(std::string_view text, uint8_t& value) {
    text = trim(text);
    if (text.empty() || text.size() >= 8) {
        return false;
    }
    char buffer[8]{};
    std::memcpy(buffer, text.data(), text.size());
    char* end = nullptr;
    const long parsed = std::strtol(buffer, &end, 10);
    if (end == buffer || *end != '\0' || parsed < 0 || parsed > 255) {
        return false;
    }
    value = static_cast<uint8_t>(parsed);
    return true;
}

// HA's standard rgb_command_topic payload: "R,G,B" (see MQTT Light docs).
bool parseRgbCsv(const std::string& payload, PixelColor& color) {
    std::string_view remaining = payload;
    uint8_t channels[3] = {0, 0, 0};
    for (int index = 0; index < 3; ++index) {
        const bool isLast = index == 2;
        const size_t comma = remaining.find(',');
        if (isLast ? comma != std::string_view::npos : comma == std::string_view::npos) {
            return false;
        }
        const std::string_view field = isLast ? remaining : remaining.substr(0, comma);
        if (!parseChannel(field, channels[index])) {
            return false;
        }
        if (!isLast) {
            remaining = remaining.substr(comma + 1);
        }
    }
    color = PixelColor{channels[0], channels[1], channels[2]};
    return true;
}

std::string setOutputColorPayload(PixelColor color) {
    return "{\"r\":" + std::to_string(color.r) + ",\"g\":" + std::to_string(color.g) + ",\"b\":" + std::to_string(color.b) + "}";
}
} // namespace

const PixelEffectSolidHaEntityAdapter& PixelEffectSolidHaEntityAdapter::instance() {
    static const PixelEffectSolidHaEntityAdapter adapter;
    return adapter;
}

DeviceTypeId PixelEffectSolidHaEntityAdapter::typeId() const {
    return PixelEffectSolidDevice::descriptor().typeId;
}

const char* PixelEffectSolidHaEntityAdapter::typeName() const {
    return "pixel_effect_solid";
}

const char* PixelEffectSolidHaEntityAdapter::haComponent() const {
    return ha::component::kLight;
}

void PixelEffectSolidHaEntityAdapter::buildDiscoveryPayload(const IDeviceRuntime& runtime, const std::string& uniqueId,
                                                            const std::string& effectiveName, const HaTopicBuilder& topicFor,
                                                            JsonObject output) const {
    (void)runtime;
    writeHaEntityIdentity(output, uniqueId, effectiveName);
    output[ha::key::kStateTopic] = topicFor(ha::component::kLight, ha::topic::kState);
    output[ha::key::kCommandTopic] = topicFor(ha::component::kLight, ha::topic::kSet);
    output[ha::key::kPayloadOn] = ha::payload::kOn;
    output[ha::key::kPayloadOff] = ha::payload::kOff;
    output["rgb_state_topic"] = topicFor("light_rgb", ha::topic::kState);
    output["rgb_command_topic"] = topicFor("light_rgb", ha::topic::kSet);
    output["optimistic"] = false;
    output[ha::key::kIcon] = "mdi:palette";
}

void PixelEffectSolidHaEntityAdapter::publishState(const IDeviceRuntime& runtime, const HaTopicBuilder& topicFor,
                                                   const HaStatePublisher& publish) const {
    const auto& effect = static_cast<const PixelEffectSolidDevice&>(runtime);
    const PixelColor color = effect.liveColor();
    // Explicit on/off gate (PixelEffectSolidDevice::liveOn()), not derived from color == black --
    // see docs/pixel-strip.md for why picking black in the color picker must not silently flip this.
    publish(topicFor(ha::component::kLight, ha::topic::kState), effect.liveOn() ? ha::payload::kOn : ha::payload::kOff);
    publish(topicFor("light_rgb", ha::topic::kState),
            std::to_string(color.r) + "," + std::to_string(color.g) + "," + std::to_string(color.b));
}

bool PixelEffectSolidHaEntityAdapter::applyCommand(DeviceRegistry& registry, const IDeviceRuntime& runtime, DeviceId deviceId,
                                                   const std::string& commandKey, const std::string& payload, uint32_t now) const {
    (void)runtime;
    if (commandKey == ha::component::kLight) {
        // Explicit on/off gate, independent of color -- see PixelEffectSolidDevice::handleCommand()'s
        // {"on": bool} SetOutput payload.
        if (equalsIgnoreCase(payload, "off")) {
            return registry.command(DeviceCommand{DeviceCommandType::SetOutput, deviceId, R"({"on":false})"}, now).ok();
        }
        if (equalsIgnoreCase(payload, "on")) {
            return registry.command(DeviceCommand{DeviceCommandType::SetOutput, deviceId, R"({"on":true})"}, now).ok();
        }
        return false;
    }
    if (commandKey == "light_rgb") {
        PixelColor color{};
        if (!parseRgbCsv(payload, color)) {
            return false;
        }
        return registry.command(DeviceCommand{DeviceCommandType::SetOutput, deviceId, setOutputColorPayload(color)}, now).ok();
    }
    return false;
}

} // namespace ewfm
