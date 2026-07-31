#include "integrations/mqtt/pixel_strip/PixelStripHaEntityAdapter.h"

#include "devices/pixel/PixelStripDevice.h"
#include "devices/pixel/PixelStripDeviceConfig.h"
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
} // namespace

const PixelStripHaEntityAdapter& PixelStripHaEntityAdapter::instance() {
    static const PixelStripHaEntityAdapter adapter;
    return adapter;
}

DeviceTypeId PixelStripHaEntityAdapter::typeId() const {
    return PixelStripDevice::descriptor().typeId;
}

const char* PixelStripHaEntityAdapter::typeName() const {
    return "pixel_strip";
}

const char* PixelStripHaEntityAdapter::haComponent() const {
    return ha::component::kLight;
}

void PixelStripHaEntityAdapter::buildDiscoveryPayload(const IDeviceRuntime& runtime, const std::string& uniqueId,
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
    // The strip's internal brightness scale is already 0..255 (Adafruit_NeoPixel::setBrightness),
    // so this matches HA's default brightness_scale 1:1 -- no conversion on publish, see below.
    output["brightness_scale"] = 255;
    output["on_command_type"] = "brightness";
    output["optimistic"] = false;
    output[ha::key::kIcon] = "mdi:led-strip-variant";
}

void PixelStripHaEntityAdapter::publishState(const IDeviceRuntime& runtime, const HaTopicBuilder& topicFor,
                                             const HaStatePublisher& publish) const {
    const IPixelStripRuntime* stripRuntime = runtime.pixelStripRuntime();
    if (stripRuntime == nullptr) {
        return;
    }
    const auto& strip = static_cast<const PixelStripDevice&>(runtime);
    // Explicit on/off gate (PixelStripDevice::liveOn()), not derived from brightness == 0 -- see
    // docs/pixel-strip.md for why the brightness slider must not be able to silently flip this.
    publish(topicFor(ha::component::kLight, ha::topic::kState), strip.liveOn() ? ha::payload::kOn : ha::payload::kOff);
    publish(topicFor("light_brightness", ha::topic::kState), std::to_string(strip.liveBrightness()));
}

bool PixelStripHaEntityAdapter::applyCommand(DeviceRegistry& registry, const IDeviceRuntime& runtime, DeviceId deviceId,
                                             const std::string& commandKey, const std::string& payload, uint32_t now) const {
    (void)runtime;
    if (commandKey == ha::component::kLight) {
        // Explicit on/off gate, independent of brightness -- see PixelStripDevice::handleCommand()'s
        // {"on": bool} SetOutput payload.
        if (equalsIgnoreCase(payload, "off")) {
            return registry.command(DeviceCommand{DeviceCommandType::SetOutput, deviceId, R"({"on":false})"}, now).ok();
        }
        if (equalsIgnoreCase(payload, "on")) {
            return registry.command(DeviceCommand{DeviceCommandType::SetOutput, deviceId, R"({"on":true})"}, now).ok();
        }
        return false;
    }
    if (commandKey == "light_brightness") {
        uint32_t brightness = 0U;
        if (!parseUnsigned(payload, brightness) || brightness > 255U) {
            return false;
        }
        const uint32_t percent = pixelBrightnessToPercent(static_cast<uint8_t>(brightness));
        return registry.command(DeviceCommand{DeviceCommandType::SetOutput, deviceId, std::to_string(percent)}, now).ok();
    }
    return false;
}

} // namespace ewfm
