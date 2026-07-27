#pragma once

#include "devices/display/DisplayLayoutProfile.h"

#include <cstdint>

namespace ewfm {

enum class Tm1637PanelKind : uint8_t {
    FourDigitDecimal036 = 0,
};

struct Tm1637PanelProfile {
    uint8_t digitCount{4U};
    DisplayAuxSegmentMode auxSegmentMode{DisplayAuxSegmentMode::PerDigitDecimalPoint};
    uint8_t supportedRotationsMask{0x05U};
};

bool tm1637PanelKindFromString(const char* value, Tm1637PanelKind& panelKind);
const char* tm1637PanelKindName(Tm1637PanelKind panelKind);
const Tm1637PanelProfile& tm1637PanelProfile(Tm1637PanelKind panelKind);

} // namespace ewfm
