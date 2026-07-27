#pragma once

#include "devices/display/render/DisplayRenderSurface.h"

#include <array>
#include <cstdint>

namespace ewfm {

constexpr uint8_t kHd44780SurfaceMaxColumns = 20U;
constexpr uint8_t kHd44780SurfaceMaxRows = 4U;

class Hd44780CharacterSurface final : public ICharacterDisplayRenderSurface {
public:
    Hd44780CharacterSurface(uint8_t columns, uint8_t rows);

    void clear(uint16_t color) override;
    void drawText(const DisplayLayoutWidgetV1& widget, const DisplayTextEvaluationResult& text) override;

    const char* line(uint8_t row) const;

private:
    void clearRow(uint8_t row);
    void writeGlyph(uint8_t x, uint8_t y, char glyph);

    uint8_t columns_{0U};
    uint8_t rows_{0U};
    std::array<std::array<char, kHd44780SurfaceMaxColumns + 1U>, kHd44780SurfaceMaxRows> lines_{};
};

} // namespace ewfm
