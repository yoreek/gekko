#pragma once

#include "devices/core/DeviceBaseConfig.h"

namespace ewfm {

template <typename ValueType> struct OutputDeviceValueCodec;

#pragma pack(push, 1)
template <typename ValueType> struct OutputDeviceConfigV1 : DeviceBaseConfigV1 {
    using StateType = ValueType;

    bool restorePreviousState{false};
    StateType startupState{};
    StateType safeState{};
    bool inverted{false};

    bool parseJson(const JsonObjectConst& input, const char*& error) {
        if (!DeviceBaseConfigV1::parseJson(input, error)) {
            return false;
        }
        restorePreviousState = input["restorePreviousState"] | restorePreviousState;
        inverted = input["inverted"] | inverted;
        if (!input["startupState"].isNull() && !OutputDeviceValueCodec<ValueType>::parseJson(input["startupState"], startupState, error)) {
            return false;
        }
        return input["safeState"].isNull() || OutputDeviceValueCodec<ValueType>::parseJson(input["safeState"], safeState, error);
    }

    DeviceValidationResult validate() const {
        const DeviceValidationResult baseValidation = DeviceBaseConfigV1::validate();
        if (!baseValidation.ok()) {
            return baseValidation;
        }
        if (!OutputDeviceValueCodec<ValueType>::valid(startupState)) {
            return {DeviceError::InvalidConfig, "output startup state is invalid"};
        }
        if (!OutputDeviceValueCodec<ValueType>::valid(safeState)) {
            return {DeviceError::InvalidConfig, "output safe state is invalid"};
        }
        return {};
    }

    void writeJson(JsonObject output) const {
        DeviceBaseConfigV1::writeJson(output);
        output["restorePreviousState"] = restorePreviousState;
        OutputDeviceValueCodec<ValueType>::writeJson(output, "startupState", startupState);
        OutputDeviceValueCodec<ValueType>::writeJson(output, "safeState", safeState);
        output["inverted"] = inverted;
    }
};
#pragma pack(pop)

} // namespace ewfm
