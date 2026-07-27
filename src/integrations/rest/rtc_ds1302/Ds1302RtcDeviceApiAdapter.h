#pragma once

#include "devices/rtc/ds1302/Ds1302RtcDevice.h"
#include "integrations/rest/common/TypedDeviceApiAdapter.h"

namespace ewfm {

class Ds1302RtcDeviceApiAdapter final : public TypedDeviceApiAdapter<Ds1302RtcDeviceApiAdapter, Ds1302RtcDevice, Ds1302RtcDeviceConfigV1> {
public:
    static constexpr const char* kTypeName = "rtc_ds1302";

    bool parseCreateExtras(const JsonObjectConst& input, const JsonObjectConst& configInput, Ds1302RtcDeviceConfigV1& config,
                           DeviceCreateRequest& request, const char*& error) const;
    bool parseUpdateExtras(const JsonObjectConst& input, const JsonObjectConst& configInput, Ds1302RtcDeviceConfigV1& config,
                           DeviceConfigUpdateRequest& request, const char*& error) const;
    void writeRuntimeJson(const Ds1302RtcDevice& device, JsonObject runtimeJson) const;

    DeviceValidationResult validateCreateRequest(const DeviceCreateRequest& request, const DeviceRegistry& registry) const override;
    DeviceValidationResult validateUpdateConfigRequest(const IDeviceRuntime& runtime, const DeviceConfigUpdateRequest& request,
                                                       const DeviceRegistry& registry) const override;
    DeviceValidationResult validateSetDepsRequest(const IDeviceRuntime& runtime,
                                                  const std::array<DeviceDependencyLink, kMaxDeviceDependencies>& deps, uint8_t depCount,
                                                  const DeviceRegistry& registry) const override;
};

} // namespace ewfm
