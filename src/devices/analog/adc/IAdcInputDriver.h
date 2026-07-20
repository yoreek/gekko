#pragma once

#include <cstdint>

namespace ewfm {

// Mirrors the ESP32 ADC attenuation levels (Arduino core's adc_attenuation_t). The ESP32 has no
// external/selectable reference voltage pin for its ADC - these four presets are the closest
// hardware equivalent, each selecting a different usable input voltage range, referenced
// internally against the factory-calibrated (eFuse) reference used by analogReadMilliVolts().
enum class AdcAttenuation : uint8_t {
    Db0 = 0,   // ~100-950 mV usable input range
    Db2_5 = 1, // ~100-1250 mV
    Db6 = 2,   // ~150-1750 mV
    Db11 = 3,  // ~150-3100 mV
};

class IAdcInputDriver {
public:
    IAdcInputDriver() = default;
    IAdcInputDriver(const IAdcInputDriver&) = delete;
    IAdcInputDriver& operator=(const IAdcInputDriver&) = delete;
    virtual ~IAdcInputDriver() = default;

    virtual bool configurePin(uint8_t pin, AdcAttenuation attenuation) = 0;
    virtual uint32_t readMilliVolts(uint8_t pin) = 0;
    virtual void release(uint8_t pin) = 0;
};

} // namespace ewfm
