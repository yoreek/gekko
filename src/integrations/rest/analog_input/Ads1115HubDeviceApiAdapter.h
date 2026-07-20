#pragma once

#include "devices/analog/input/ads1115/Ads1115HubDevice.h"
#include "integrations/rest/common/TypedDeviceApiAdapter.h"

namespace ewfm {

class Ads1115HubDeviceApiAdapter final
    : public TypedDeviceApiAdapter<Ads1115HubDeviceApiAdapter, Ads1115HubDevice, Ads1115HubDeviceConfigV1> {
public:
    static constexpr const char* kTypeName = "ads1115_hub";

    bool parseCreateExtras(const JsonObjectConst& input, const JsonObjectConst& configInput, Ads1115HubDeviceConfigV1& config,
                           DeviceCreateRequest& request, const char*& error) const;
    bool parseUpdateExtras(const JsonObjectConst& input, const JsonObjectConst& configInput, Ads1115HubDeviceConfigV1& config,
                           DeviceConfigUpdateRequest& request, const char*& error) const;

    DeviceValidationResult validateCreateRequest(const DeviceCreateRequest& request, const DeviceRegistry& registry) const override;
    DeviceValidationResult validateUpdateConfigRequest(const IDeviceRuntime& runtime, const DeviceConfigUpdateRequest& request,
                                                       const DeviceRegistry& registry) const override;
    DeviceValidationResult validateSetDepsRequest(const IDeviceRuntime& runtime,
                                                  const std::array<DeviceDependencyLink, kMaxDeviceDependencies>& deps, uint8_t depCount,
                                                  const DeviceRegistry& registry) const override;
};

} // namespace ewfm
