#pragma once

#include "devices/core/DeviceTypes.h"

#include <algorithm>
#include <cstdint>

namespace ewfm {

// Nominal ADC reference used only to derive the best-effort, UI-facing `rawCode` from the
// authoritative `milliVolts` reading -- see AnalogInputReading's field comments. Shared by every
// AnalogInput backend (on-chip port, ADS1115 hub, CD74HC4067 hub) so the normalization is uniform
// regardless of which one produced the sample.
constexpr int32_t kAnalogInputNominalReferenceMilliVolts = 3300;

inline uint16_t analogInputRawCode(int32_t milliVolts, int32_t referenceMilliVolts = kAnalogInputNominalReferenceMilliVolts) {
    const int32_t scaled = (milliVolts * static_cast<int32_t>(kAnalogInputResolutionMax)) / referenceMilliVolts;
    return static_cast<uint16_t>(std::clamp<int32_t>(scaled, 0, static_cast<int32_t>(kAnalogInputResolutionMax)));
}

} // namespace ewfm
