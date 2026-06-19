#include "devices/sensors/temperature/TemperatureSensorTypes.h"

#include <cstdlib>
#include <cstring>

namespace ewfm {

bool temperatureUnitFromString(const char* value, TemperatureUnit& unit) {
    if (value == nullptr || std::strcmp(value, "celsius") == 0) {
        unit = TemperatureUnit::Celsius;
        return true;
    }
    if (std::strcmp(value, "fahrenheit") == 0) {
        unit = TemperatureUnit::Fahrenheit;
        return true;
    }
    return false;
}

bool temperatureUnitFromByte(uint8_t value, TemperatureUnit& unit) {
    switch (value) {
    case static_cast<uint8_t>(TemperatureUnit::Celsius):
        unit = TemperatureUnit::Celsius;
        return true;
    case static_cast<uint8_t>(TemperatureUnit::Fahrenheit):
        unit = TemperatureUnit::Fahrenheit;
        return true;
    default:
        return false;
    }
}

uint8_t temperatureUnitToByte(TemperatureUnit unit) {
    return static_cast<uint8_t>(unit);
}

const char* temperatureUnitName(TemperatureUnit unit) {
    switch (unit) {
    case TemperatureUnit::Celsius:
        return "celsius";
    case TemperatureUnit::Fahrenheit:
        return "fahrenheit";
    }
    return "celsius";
}

const char* temperatureUnitSymbol(TemperatureUnit unit) {
    switch (unit) {
    case TemperatureUnit::Celsius:
        return "C";
    case TemperatureUnit::Fahrenheit:
        return "F";
    }
    return "C";
}

int32_t convertMilliCelsiusToUnit(int32_t milliCelsius, TemperatureUnit unit) {
    if (unit == TemperatureUnit::Fahrenheit) {
        return static_cast<int32_t>((static_cast<int64_t>(milliCelsius) * 9) / 5 + 32000);
    }
    return milliCelsius;
}

bool temperatureReadingChanged(const TemperatureReading& previous, const TemperatureReading& current, uint16_t reportDeltaCentiCelsius) {
    if (previous.valid != current.valid) {
        return true;
    }
    if (!current.valid) {
        return false;
    }

    const int32_t deltaMilliCelsius = std::abs(current.milliCelsius - previous.milliCelsius);
    return deltaMilliCelsius >= static_cast<int32_t>(reportDeltaCentiCelsius) * 10;
}

void writeTemperatureOutputJson(const TemperatureReading& reading, TemperatureUnit unit, const char* status, JsonObject output) {
    const int32_t displayMilli = reading.valid ? convertMilliCelsiusToUnit(reading.milliCelsius, unit) : 0;
    output["value"] = static_cast<float>(displayMilli) / 1000.0F;
    output["unit"] = temperatureUnitName(unit);
    output["unit_symbol"] = temperatureUnitSymbol(unit);
    output["measured_at_ms"] = reading.valid ? reading.measuredAtMs : 0U;
    output["valid"] = reading.valid;
    output["status"] = status != nullptr ? status : (reading.valid ? "ok" : "not_ready");
}

} // namespace ewfm
