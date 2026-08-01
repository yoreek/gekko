#include "devices/analog/adc/ArduinoAdcInputDriver.h"

#if defined(ARDUINO)
#include <Arduino.h>
#endif

namespace ewfm {

namespace {
#if defined(ARDUINO)
adc_attenuation_t toArduinoAttenuation(AdcAttenuation attenuation) {
    switch (attenuation) {
    case AdcAttenuation::Db0:
        return ADC_0db;
    case AdcAttenuation::Db2_5:
        return ADC_2_5db;
    case AdcAttenuation::Db6:
        return ADC_6db;
    case AdcAttenuation::Db11:
        return ADC_11db;
    }
    return ADC_11db;
}
#endif
} // namespace

bool ArduinoAdcInputDriver::configurePin(uint8_t pin, AdcAttenuation attenuation) {
#if defined(ARDUINO)
    pinMode(pin, INPUT);
#if ESP_ARDUINO_VERSION_MAJOR < 3
    // Core 3.x attaches the ADC channel implicitly on first read; only 2.x needs this.
    if (!adcAttachPin(pin)) {
        return false;
    }
#endif
    analogSetPinAttenuation(pin, toArduinoAttenuation(attenuation));
    return true;
#else
    (void)pin;
    (void)attenuation;
    return true;
#endif
}

uint32_t ArduinoAdcInputDriver::readMilliVolts(uint8_t pin) {
#if defined(ARDUINO)
    return analogReadMilliVolts(pin);
#else
    (void)pin;
    return 0;
#endif
}

void ArduinoAdcInputDriver::release(uint8_t pin) {
#if defined(ARDUINO)
    pinMode(pin, INPUT);
#else
    (void)pin;
#endif
}

ArduinoAdcInputDriver& defaultArduinoAdcInputDriver() {
    static ArduinoAdcInputDriver driver;
    return driver;
}

} // namespace ewfm
