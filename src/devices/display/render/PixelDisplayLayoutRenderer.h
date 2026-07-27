#pragma once

#include "devices/display/render/DisplayRenderSurface.h"

namespace ewfm {

class PixelDisplayLayoutRenderer final {
public:
    static constexpr bool kSupportsText = true;
    static constexpr bool kSupportsDigital = false;
    static constexpr bool kSupportsShapes = true;
    static constexpr bool kSupportsBitmap = true;

    explicit PixelDisplayLayoutRenderer(IPixelDisplayRenderSurface& surface) : surface_(surface) {}

    void clear(uint16_t color) {
        surface_.clear(color);
    }

    void drawText(const DisplayLayoutWidgetV1& widget, const DisplayTextEvaluationResult& text) {
        surface_.drawText(widget, text);
    }

    void drawRect(const DisplayLayoutWidgetV1& widget) {
        surface_.drawRect(widget);
    }

    void drawLine(const DisplayLayoutWidgetV1& widget) {
        surface_.drawLine(widget);
    }

    void drawCircle(const DisplayLayoutWidgetV1& widget) {
        surface_.drawCircle(widget);
    }

    void drawEllipse(const DisplayLayoutWidgetV1& widget) {
        surface_.drawEllipse(widget);
    }

    void drawBitmap(const DisplayLayoutWidgetV1& widget) {
        surface_.drawBitmap(widget);
    }

private:
    IPixelDisplayRenderSurface& surface_;
};

} // namespace ewfm
