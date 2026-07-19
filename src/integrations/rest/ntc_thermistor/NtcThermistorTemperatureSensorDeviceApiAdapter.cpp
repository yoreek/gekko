#include "integrations/rest/ntc_thermistor/NtcThermistorTemperatureSensorDeviceApiAdapter.h"

namespace ewfm {

void NtcThermistorTemperatureSensorDeviceApiAdapter::writeRuntimeJson(const NtcThermistorTemperatureSensorDevice& device,
                                                                      JsonObject runtimeJson) const {
    JsonObject outputJson = runtimeJson.createNestedObject("output");
    JsonObject temperature = outputJson.createNestedObject("temperature");
    writeTemperatureOutputJson(device.reading(),
                               device.config().outputUnit == temperatureUnitToByte(TemperatureUnit::Fahrenheit)
                                   ? TemperatureUnit::Fahrenheit
                                   : TemperatureUnit::Celsius,
                               device.outputStatus(), temperature);
}

} // namespace ewfm
