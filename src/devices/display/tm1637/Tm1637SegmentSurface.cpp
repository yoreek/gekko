#include "devices/display/tm1637/Tm1637SegmentSurface.h"

#include <cstring>

namespace ewfm {

Tm1637SegmentSurface::Tm1637SegmentSurface(const uint8_t digitCount) : digitCount_(digitCount) {
    clear(0U);
}

void Tm1637SegmentSurface::clear(uint16_t) {
    frame_ = {};
    frame_.cellCount = digitCount_;
    for (uint8_t index = 0U; index < digitCount_ && index < Tm1637SegmentCodec::kDigitCount; ++index) {
        frame_.cells[index].glyph = ' ';
        frame_.cells[index].decimalPoint = 0U;
    }
}

void Tm1637SegmentSurface::drawDigital(const DisplayLayoutWidgetV1& widget, const DisplayDigitalFrame& frame) {
    if (widget.width == 0U || widget.x >= digitCount_) {
        return;
    }
    const uint8_t maxWidth = static_cast<uint8_t>(digitCount_ - widget.x);
    const uint8_t copyCount = frame.cellCount < maxWidth ? frame.cellCount : maxWidth;
    for (uint8_t index = 0U; index < copyCount; ++index) {
        frame_.cells[widget.x + index] = frame.cells[index];
    }
}

bool Tm1637SegmentSurface::snapshot(DisplayDigitalFrame& frame) const {
    frame = frame_;
    return true;
}

} // namespace ewfm
