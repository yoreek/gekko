#pragma once

#include "devices/display/DisplayLayoutStore.h"
#include "devices/display/DisplayTextEvaluator.h"

#include <cstdint>

namespace ewfm {

struct DisplayDigitalCell {
    char glyph{' '};
    uint8_t decimalPoint{0U};
};

struct DisplayDigitalFrame {
    uint8_t cellCount{0U};
    DisplayDigitalCell cells[8]{};
    bool overflowed{false};
};

bool buildDisplayDigitalFrame(const DisplayLayoutWidgetV1& widget, const DisplayTextEvaluationResult& text, DisplayDigitalFrame& frame);

} // namespace ewfm
