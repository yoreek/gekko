#pragma once

#include "devices/switch/auto/AutoSwitchDevice.h"
#include "integrations/rest/common/TypedDeviceApiAdapter.h"

namespace ewfm {

class AutoSwitchDeviceApiAdapter final
    : public TypedDeviceApiAdapter<AutoSwitchDeviceApiAdapter, AutoSwitchDevice, AutoSwitchDeviceConfigV1> {
public:
    static constexpr const char* kTypeName = "auto_switch";

    bool parseCreateExtras(const JsonObjectConst& input, const JsonObjectConst& configInput, AutoSwitchDeviceConfigV1& config,
                           DeviceCreateRequest& request, const char*& error) const;
    bool parseUpdateExtras(const JsonObjectConst& input, const JsonObjectConst& configInput, AutoSwitchDeviceConfigV1& config,
                           DeviceConfigUpdateRequest& request, const char*& error) const;
    void writeRuntimeJson(const AutoSwitchDevice& device, JsonObject runtimeJson) const;

    DeviceValidationResult validateCreateRequest(const DeviceCreateRequest& request, const DeviceRegistry& registry) const override;
    DeviceValidationResult validateUpdateConfigRequest(const IDeviceRuntime& runtime, const DeviceConfigUpdateRequest& request,
                                                       const DeviceRegistry& registry) const override;
    DeviceValidationResult validateSetDepsRequest(const IDeviceRuntime& runtime,
                                                  const std::array<DeviceDependencyLink, kMaxDeviceDependencies>& deps, uint8_t depCount,
                                                  const DeviceRegistry& registry) const override;
};

} // namespace ewfm
