#pragma once

#include "devices/rtc/ds3231/Ds3231RtcDevice.h"
#include "integrations/rest/common/TypedDeviceApiAdapter.h"

namespace ewfm {

class Ds3231RtcDeviceApiAdapter final : public TypedDeviceApiAdapter<Ds3231RtcDeviceApiAdapter, Ds3231RtcDevice, Ds3231RtcDeviceConfigV2> {
public:
    static constexpr const char* kTypeName = "rtc_ds3231";

    bool parseCreateExtras(const JsonObjectConst& input, const JsonObjectConst& configInput, Ds3231RtcDeviceConfigV2& config,
                           DeviceCreateRequest& request, const char*& error) const;
    bool parseUpdateExtras(const JsonObjectConst& input, const JsonObjectConst& configInput, Ds3231RtcDeviceConfigV2& config,
                           DeviceConfigUpdateRequest& request, const char*& error) const;
    void writeRuntimeJson(const Ds3231RtcDevice& device, JsonObject runtimeJson) const;

    DeviceValidationResult validateCreateRequest(const DeviceCreateRequest& request, const DeviceRegistry& registry) const override;
    DeviceValidationResult validateUpdateConfigRequest(const IDeviceRuntime& runtime, const DeviceConfigUpdateRequest& request,
                                                       const DeviceRegistry& registry) const override;
    DeviceValidationResult validateSetDepsRequest(const IDeviceRuntime& runtime,
                                                  const std::array<DeviceDependencyLink, kMaxDeviceDependencies>& deps, uint8_t depCount,
                                                  const DeviceRegistry& registry) const override;
};

} // namespace ewfm
