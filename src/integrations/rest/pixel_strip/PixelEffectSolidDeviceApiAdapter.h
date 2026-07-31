#pragma once

#include "devices/pixel/effects/PixelEffectSolidDevice.h"
#include "devices/pixel/effects/PixelEffectSolidDeviceConfig.h"
#include "integrations/rest/common/TypedDeviceApiAdapter.h"

namespace ewfm {

class PixelEffectSolidDeviceApiAdapter final
    : public TypedDeviceApiAdapter<PixelEffectSolidDeviceApiAdapter, PixelEffectSolidDevice, PixelEffectSolidDeviceConfigV1> {
public:
    static constexpr const char* kTypeName = "pixel_effect_solid";

    bool parseCreateExtras(const JsonObjectConst& input, const JsonObjectConst& configInput, PixelEffectSolidDeviceConfigV1& config,
                           DeviceCreateRequest& request, const char*& error) const;
    bool parseUpdateExtras(const JsonObjectConst& input, const JsonObjectConst& configInput, PixelEffectSolidDeviceConfigV1& config,
                           DeviceConfigUpdateRequest& request, const char*& error) const;
    void writeRuntimeJson(const PixelEffectSolidDevice& device, JsonObject runtimeJson) const;

    DeviceValidationResult validateCreateRequest(const DeviceCreateRequest& request, const DeviceRegistry& registry) const override;
    DeviceValidationResult validateUpdateConfigRequest(const IDeviceRuntime& runtime, const DeviceConfigUpdateRequest& request,
                                                       const DeviceRegistry& registry) const override;
    DeviceValidationResult validateSetDepsRequest(const IDeviceRuntime& runtime,
                                                  const std::array<DeviceDependencyLink, kMaxDeviceDependencies>& deps, uint8_t depCount,
                                                  const DeviceRegistry& registry) const override;
};

} // namespace ewfm
