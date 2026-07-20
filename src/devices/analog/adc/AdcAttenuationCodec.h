#pragma once

#include "devices/analog/adc/IAdcInputDriver.h"

namespace ewfm {

bool attenuationFromByte(uint8_t value, AdcAttenuation& attenuation);
bool attenuationFromString(const char* value, AdcAttenuation& attenuation);
const char* attenuationName(AdcAttenuation attenuation);

} // namespace ewfm
