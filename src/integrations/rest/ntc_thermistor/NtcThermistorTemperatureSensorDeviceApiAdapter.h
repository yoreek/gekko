#pragma once

#include "devices/sensors/ntc_thermistor/NtcThermistorTemperatureSensorDevice.h"
#include "integrations/rest/common/TypedDeviceApiAdapter.h"

namespace ewfm {

class NtcThermistorTemperatureSensorDeviceApiAdapter final
    : public TypedDeviceApiAdapter<NtcThermistorTemperatureSensorDeviceApiAdapter, NtcThermistorTemperatureSensorDevice,
                                   NtcThermistorTemperatureSensorConfigV1> {
public:
    static constexpr const char* kTypeName = "ntc_thermistor_temperature_sensor";

    void writeRuntimeJson(const NtcThermistorTemperatureSensorDevice& device, JsonObject runtimeJson) const;
};

} // namespace ewfm
