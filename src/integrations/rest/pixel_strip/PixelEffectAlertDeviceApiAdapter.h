#pragma once

#include "devices/pixel/effects/PixelEffectAlertDevice.h"
#include "devices/pixel/effects/PixelEffectAlertDeviceConfig.h"
#include "integrations/rest/common/TypedDeviceApiAdapter.h"

namespace ewfm {

class PixelEffectAlertDeviceApiAdapter final
    : public TypedDeviceApiAdapter<PixelEffectAlertDeviceApiAdapter, PixelEffectAlertDevice, PixelEffectAlertDeviceConfigV1> {
public:
    static constexpr const char* kTypeName = "pixel_effect_alert";

    bool parseCreateExtras(const JsonObjectConst& input, const JsonObjectConst& configInput, PixelEffectAlertDeviceConfigV1& config,
                           DeviceCreateRequest& request, const char*& error) const;
    bool parseUpdateExtras(const JsonObjectConst& input, const JsonObjectConst& configInput, PixelEffectAlertDeviceConfigV1& config,
                           DeviceConfigUpdateRequest& request, const char*& error) const;
    void writeRuntimeJson(const PixelEffectAlertDevice& device, JsonObject runtimeJson) const;

    DeviceValidationResult validateCreateRequest(const DeviceCreateRequest& request, const DeviceRegistry& registry) const override;
    DeviceValidationResult validateUpdateConfigRequest(const IDeviceRuntime& runtime, const DeviceConfigUpdateRequest& request,
                                                       const DeviceRegistry& registry) const override;
    DeviceValidationResult validateSetDepsRequest(const IDeviceRuntime& runtime,
                                                  const std::array<DeviceDependencyLink, kMaxDeviceDependencies>& deps, uint8_t depCount,
                                                  const DeviceRegistry& registry) const override;
};

} // namespace ewfm
