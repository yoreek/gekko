#pragma once

#include <ArduinoJson.h>
#include <cstdint>

namespace ewfm {

enum class TemperatureUnit : uint8_t {
    Celsius = 0,
    Fahrenheit = 1,
};

struct TemperatureReading {
    int32_t milliCelsius{0};
    uint32_t measuredAtMs{0};
    bool valid{false};
};

bool temperatureUnitFromString(const char* value, TemperatureUnit& unit);
bool temperatureUnitFromByte(uint8_t value, TemperatureUnit& unit);
uint8_t temperatureUnitToByte(TemperatureUnit unit);
const char* temperatureUnitName(TemperatureUnit unit);
const char* temperatureUnitSymbol(TemperatureUnit unit);
int32_t convertMilliCelsiusToUnit(int32_t milliCelsius, TemperatureUnit unit);
bool temperatureReadingChanged(const TemperatureReading& previous, const TemperatureReading& current, uint16_t reportDeltaCentiCelsius);
void writeTemperatureOutputJson(const TemperatureReading& reading, TemperatureUnit unit, const char* status, JsonObject output);

} // namespace ewfm
