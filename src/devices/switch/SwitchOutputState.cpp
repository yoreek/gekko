#include "devices/switch/SwitchOutputState.h"

#include "devices/core/DeviceTypes.h"
#include "devices/switch/SwitchDeviceConfig.h"

#include <ArduinoJson.h>
#include <string_view>

namespace ewfm {

bool parseSwitchOutputStateCommand(const DeviceCommand& command, bool& state) {
    if (command.type != DeviceCommandType::SetOutput && command.type != DeviceCommandType::Custom) {
        return false;
    }
    StaticJsonDocument<96> doc;
    const std::string_view payload = command.payload.view();
    if (deserializeJson(doc, payload.data(), payload.size())) {
        return false;
    }
    const char* error = nullptr;
    return OutputDeviceValueCodec<bool>::parseJson(doc.as<JsonVariantConst>(), state, error);
}

const char* switchOutputStateName(const bool state) {
    return state ? "on" : "off";
}

} // namespace ewfm
