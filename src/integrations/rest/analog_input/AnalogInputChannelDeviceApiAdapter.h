#pragma once

#include "devices/analog/input/channel/AnalogInputChannelDevice.h"
#include "integrations/rest/common/TypedDeviceApiAdapter.h"

namespace ewfm {

class AnalogInputChannelDeviceApiAdapter final
    : public TypedDeviceApiAdapter<AnalogInputChannelDeviceApiAdapter, AnalogInputChannelDevice, AnalogInputChannelDeviceConfigV1> {
public:
    static constexpr const char* kTypeName = "analog_input_channel";

    bool parseCreateExtras(const JsonObjectConst& input, const JsonObjectConst& configInput, AnalogInputChannelDeviceConfigV1& config,
                           DeviceCreateRequest& request, const char*& error) const;
    bool parseUpdateExtras(const JsonObjectConst& input, const JsonObjectConst& configInput, AnalogInputChannelDeviceConfigV1& config,
                           DeviceConfigUpdateRequest& request, const char*& error) const;
    void writeRuntimeJson(const AnalogInputChannelDevice& device, JsonObject runtimeJson) const;

    DeviceValidationResult validateCreateRequest(const DeviceCreateRequest& request, const DeviceRegistry& registry) const override;
    DeviceValidationResult validateUpdateConfigRequest(const IDeviceRuntime& runtime, const DeviceConfigUpdateRequest& request,
                                                       const DeviceRegistry& registry) const override;
    DeviceValidationResult validateSetDepsRequest(const IDeviceRuntime& runtime,
                                                  const std::array<DeviceDependencyLink, kMaxDeviceDependencies>& deps, uint8_t depCount,
                                                  const DeviceRegistry& registry) const override;
};

} // namespace ewfm
