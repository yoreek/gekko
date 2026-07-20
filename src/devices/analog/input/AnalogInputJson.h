#pragma once

#include "devices/core/DeviceTypes.h"

#include <ArduinoJson.h>

namespace ewfm {

// Shared runtime JSON shape for every AnalogInput-role leaf device (analog_port_input,
// ads1115_input, cd74hc4067_input) -- one universal `output.analogInput` object regardless of
// which backend produced the reading, mirroring how writeTemperatureOutputJson is shared by every
// temperature sensor.
void writeAnalogInputOutputJson(const AnalogInputReading& reading, const char* status, JsonObject output);

} // namespace ewfm
