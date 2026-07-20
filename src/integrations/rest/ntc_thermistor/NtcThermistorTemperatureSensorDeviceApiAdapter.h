#pragma once

#include "devices/sensors/ntc_thermistor/NtcThermistorTemperatureSensorDevice.h"
#include "integrations/rest/common/TypedDeviceApiAdapter.h"

namespace ewfm {

class NtcThermistorTemperatureSensorDeviceApiAdapter final
    : public TypedDeviceApiAdapter<NtcThermistorTemperatureSensorDeviceApiAdapter, NtcThermistorTemperatureSensorDevice,
                                   NtcThermistorTemperatureSensorConfigV1> {
public:
    static constexpr const char* kTypeName = "ntc_thermistor_temperature_sensor";

    bool parseCreateExtras(const JsonObjectConst& input, const JsonObjectConst& configInput, NtcThermistorTemperatureSensorConfigV1& config,
                           DeviceCreateRequest& request, const char*& error) const;
    bool parseUpdateExtras(const JsonObjectConst& input, const JsonObjectConst& configInput, NtcThermistorTemperatureSensorConfigV1& config,
                           DeviceConfigUpdateRequest& request, const char*& error) const;
    void writeRuntimeJson(const NtcThermistorTemperatureSensorDevice& device, JsonObject runtimeJson) const;

    DeviceValidationResult validateCreateRequest(const DeviceCreateRequest& request, const DeviceRegistry& registry) const override;
    DeviceValidationResult validateUpdateConfigRequest(const IDeviceRuntime& runtime, const DeviceConfigUpdateRequest& request,
                                                       const DeviceRegistry& registry) const override;
    DeviceValidationResult validateSetDepsRequest(const IDeviceRuntime& runtime,
                                                  const std::array<DeviceDependencyLink, kMaxDeviceDependencies>& deps, uint8_t depCount,
                                                  const DeviceRegistry& registry) const override;
};

} // namespace ewfm
