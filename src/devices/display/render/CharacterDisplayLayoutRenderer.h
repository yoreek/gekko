#pragma once

#include "devices/display/render/DisplayRenderSurface.h"

namespace ewfm {

class CharacterDisplayLayoutRenderer final {
public:
    static constexpr bool kSupportsText = true;
    static constexpr bool kSupportsDigital = false;
    static constexpr bool kSupportsShapes = false;
    static constexpr bool kSupportsBitmap = false;

    explicit CharacterDisplayLayoutRenderer(ICharacterDisplayRenderSurface& surface) : surface_(surface) {}

    void clear(uint16_t color) {
        surface_.clear(color);
    }

    void drawText(const DisplayLayoutWidgetV1& widget, const DisplayTextEvaluationResult& text) {
        surface_.drawText(widget, text);
    }

private:
    ICharacterDisplayRenderSurface& surface_;
};

} // namespace ewfm
