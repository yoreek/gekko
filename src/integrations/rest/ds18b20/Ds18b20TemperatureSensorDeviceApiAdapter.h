#pragma once

#include "devices/sensors/ds18b20/Ds18b20TemperatureSensorDevice.h"
#include "integrations/rest/common/TypedDeviceApiAdapter.h"

namespace ewfm {

class Ds18b20TemperatureSensorDeviceApiAdapter final
    : public TypedDeviceApiAdapter<Ds18b20TemperatureSensorDeviceApiAdapter, Ds18b20TemperatureSensorDevice,
                                   Ds18b20TemperatureSensorConfigV1> {
public:
    static constexpr const char* kTypeName = "ds18b20_temperature_sensor";

    bool parseCreateExtras(const JsonObjectConst& input, const JsonObjectConst& configInput, Ds18b20TemperatureSensorConfigV1& config,
                           DeviceCreateRequest& request, const char*& error) const;
    bool parseUpdateExtras(const JsonObjectConst& input, const JsonObjectConst& configInput, Ds18b20TemperatureSensorConfigV1& config,
                           DeviceConfigUpdateRequest& request, const char*& error) const;
    void writeRuntimeJson(const Ds18b20TemperatureSensorDevice& device, JsonObject runtimeJson) const;

    DeviceValidationResult validateCreateRequest(const DeviceCreateRequest& request, const DeviceRegistry& registry) const override;
    DeviceValidationResult validateUpdateConfigRequest(const IDeviceRuntime& runtime, const DeviceConfigUpdateRequest& request,
                                                       const DeviceRegistry& registry) const override;
    DeviceValidationResult validateSetDepsRequest(const IDeviceRuntime& runtime,
                                                  const std::array<DeviceDependencyLink, kMaxDeviceDependencies>& deps, uint8_t depCount,
                                                  const DeviceRegistry& registry) const override;
};

} // namespace ewfm
