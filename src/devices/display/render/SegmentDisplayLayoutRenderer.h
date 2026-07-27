#pragma once

#include "devices/display/render/DisplayRenderSurface.h"

namespace ewfm {

class SegmentDisplayLayoutRenderer final {
public:
    static constexpr bool kSupportsText = false;
    static constexpr bool kSupportsDigital = true;
    static constexpr bool kSupportsShapes = false;
    static constexpr bool kSupportsBitmap = false;

    explicit SegmentDisplayLayoutRenderer(ISegmentDisplayRenderSurface& surface) : surface_(surface) {}

    void clear(uint16_t color) {
        surface_.clear(color);
    }

    void drawDigital(const DisplayLayoutWidgetV1& widget, const DisplayDigitalFrame& frame) {
        surface_.drawDigital(widget, frame);
    }

private:
    ISegmentDisplayRenderSurface& surface_;
};

} // namespace ewfm
