#include "devices/display/tm1637/Tm1637PanelProfile.h"

#include <cstring>

namespace ewfm {
namespace {
constexpr Tm1637PanelProfile kFourDigitDecimal036Profile{4U, DisplayAuxSegmentMode::PerDigitDecimalPoint, 0x05U};
} // namespace

bool tm1637PanelKindFromString(const char* value, Tm1637PanelKind& panelKind) {
    if (value == nullptr) {
        panelKind = Tm1637PanelKind::FourDigitDecimal036;
        return true;
    }
    if (std::strcmp(value, "four_digit_decimal_036") == 0) {
        panelKind = Tm1637PanelKind::FourDigitDecimal036;
        return true;
    }
    return false;
}

const char* tm1637PanelKindName(const Tm1637PanelKind panelKind) {
    switch (panelKind) {
    case Tm1637PanelKind::FourDigitDecimal036:
    default:
        return "four_digit_decimal_036";
    }
}

const Tm1637PanelProfile& tm1637PanelProfile(const Tm1637PanelKind panelKind) {
    switch (panelKind) {
    case Tm1637PanelKind::FourDigitDecimal036:
    default:
        return kFourDigitDecimal036Profile;
    }
}

} // namespace ewfm
