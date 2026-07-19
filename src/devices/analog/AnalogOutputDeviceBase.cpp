#include "devices/analog/AnalogOutputDeviceBase.h"

#include <ArduinoJson.h>
#include <string_view>

namespace ewfm {

AnalogOutputDeviceBase::AnalogOutputDeviceBase(const AnalogOutputDeviceConfigV1& config)
    : AbstractOutputDevice<uint16_t, IAnalogOutputRuntime>(config.startupState) {}

bool AnalogOutputDeviceBase::stateIsValid(const StateType state) const {
    return state <= kAnalogOutputLevelMax;
}

uint16_t AnalogOutputDeviceBase::invertedState(const uint16_t state) const {
    return static_cast<uint16_t>(kAnalogOutputLevelMax - state);
}

bool AnalogOutputDeviceBase::parseOutputCommand(const DeviceCommand& command, StateType& state) const {
    if (command.type != DeviceCommandType::SetOutput && command.type != DeviceCommandType::Custom) {
        return false;
    }
    StaticJsonDocument<96> doc;
    const std::string_view payload = command.payload.view();
    if (deserializeJson(doc, payload.data(), payload.size())) {
        return false;
    }
    const char* error = nullptr;
    return OutputDeviceValueCodec<uint16_t>::parseJson(doc.as<JsonVariantConst>(), state, error);
}

} // namespace ewfm
