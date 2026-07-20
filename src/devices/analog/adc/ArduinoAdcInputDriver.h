#pragma once

#include "devices/analog/adc/IAdcInputDriver.h"

namespace ewfm {

class ArduinoAdcInputDriver final : public IAdcInputDriver {
public:
    bool configurePin(uint8_t pin, AdcAttenuation attenuation) override;
    uint32_t readMilliVolts(uint8_t pin) override;
    void release(uint8_t pin) override;
};

ArduinoAdcInputDriver& defaultArduinoAdcInputDriver();

} // namespace ewfm
