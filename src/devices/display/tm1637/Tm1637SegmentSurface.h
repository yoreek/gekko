#pragma once

#include "devices/display/render/DisplayRenderSurface.h"
#include "devices/display/tm1637/Tm1637SegmentCodec.h"

namespace ewfm {

class Tm1637SegmentSurface final : public ISegmentDisplayRenderSurface {
public:
    explicit Tm1637SegmentSurface(uint8_t digitCount);

    void clear(uint16_t color) override;
    void drawDigital(const DisplayLayoutWidgetV1& widget, const DisplayDigitalFrame& frame) override;

    bool snapshot(DisplayDigitalFrame& frame) const;

private:
    uint8_t digitCount_{0U};
    DisplayDigitalFrame frame_{};
};

} // namespace ewfm
