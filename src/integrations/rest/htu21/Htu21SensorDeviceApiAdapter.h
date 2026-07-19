#pragma once

#include "devices/sensors/htu21/Htu21SensorDevice.h"
#include "integrations/rest/common/TypedDeviceApiAdapter.h"

namespace ewfm {

class Htu21SensorDeviceApiAdapter final
    : public TypedDeviceApiAdapter<Htu21SensorDeviceApiAdapter, Htu21SensorDevice, Htu21SensorConfigV3> {
public:
    static constexpr const char* kTypeName = "htu21";

    bool parseCreateExtras(const JsonObjectConst& input, const JsonObjectConst& configInput, Htu21SensorConfigV3& config,
                           DeviceCreateRequest& request, const char*& error) const;
    bool parseUpdateExtras(const JsonObjectConst& input, const JsonObjectConst& configInput, Htu21SensorConfigV3& config,
                           DeviceConfigUpdateRequest& request, const char*& error) const;
    void writeRuntimeJson(const Htu21SensorDevice& device, JsonObject runtimeJson) const;

    DeviceValidationResult validateCreateRequest(const DeviceCreateRequest& request, const DeviceRegistry& registry) const override;
    DeviceValidationResult validateUpdateConfigRequest(const IDeviceRuntime& runtime, const DeviceConfigUpdateRequest& request,
                                                       const DeviceRegistry& registry) const override;
    DeviceValidationResult validateSetDepsRequest(const IDeviceRuntime& runtime,
                                                  const std::array<DeviceDependencyLink, kMaxDeviceDependencies>& deps, uint8_t depCount,
                                                  const DeviceRegistry& registry) const override;
};

} // namespace ewfm
