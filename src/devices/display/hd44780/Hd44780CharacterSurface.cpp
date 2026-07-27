#include "devices/display/hd44780/Hd44780CharacterSurface.h"

#include <algorithm>
#include <cstring>

namespace ewfm {

Hd44780CharacterSurface::Hd44780CharacterSurface(const uint8_t columns, const uint8_t rows) : columns_(columns), rows_(rows) {
    clear(0U);
}

void Hd44780CharacterSurface::clear(const uint16_t color) {
    (void)color;
    for (uint8_t row = 0U; row < rows_ && row < kHd44780SurfaceMaxRows; ++row) {
        clearRow(row);
    }
}

void Hd44780CharacterSurface::clearRow(const uint8_t row) {
    if (row >= kHd44780SurfaceMaxRows) {
        return;
    }
    const uint8_t width = std::min<uint8_t>(columns_, kHd44780SurfaceMaxColumns);
    for (uint8_t column = 0U; column < width; ++column) {
        lines_[row][column] = ' ';
    }
    lines_[row][width] = '\0';
}

void Hd44780CharacterSurface::writeGlyph(const uint8_t x, const uint8_t y, const char glyph) {
    if (x >= columns_ || y >= rows_ || x >= kHd44780SurfaceMaxColumns || y >= kHd44780SurfaceMaxRows) {
        return;
    }
    lines_[y][x] = glyph;
}

void Hd44780CharacterSurface::drawText(const DisplayLayoutWidgetV1& widget, const DisplayTextEvaluationResult& text) {
    const char* source = text.available ? text.text : "";
    uint8_t cursorX = widget.x;
    uint8_t cursorY = widget.y;
    const uint8_t maxX =
        static_cast<uint8_t>(std::min<uint16_t>(static_cast<uint16_t>(columns_), static_cast<uint16_t>(widget.x) + widget.width));
    const uint8_t maxY =
        static_cast<uint8_t>(std::min<uint16_t>(static_cast<uint16_t>(rows_), static_cast<uint16_t>(widget.y) + widget.height));
    const bool wrap = (widget.styleFlags & 0x04U) != 0U;
    for (const char* cursor = source; *cursor != '\0' && cursorY < maxY; ++cursor) {
        if (*cursor == '\n') {
            cursorX = widget.x;
            ++cursorY;
            continue;
        }
        if (cursorX < maxX) {
            writeGlyph(cursorX, cursorY, *cursor);
        }
        ++cursorX;
        if (cursorX >= maxX) {
            if (!wrap) {
                break;
            }
            cursorX = widget.x;
            ++cursorY;
        }
    }
    for (uint8_t row = 0U; row < rows_ && row < kHd44780SurfaceMaxRows; ++row) {
        if (lines_[row][std::min<uint8_t>(columns_, kHd44780SurfaceMaxColumns)] != '\0') {
            lines_[row][std::min<uint8_t>(columns_, kHd44780SurfaceMaxColumns)] = '\0';
        }
    }
}

const char* Hd44780CharacterSurface::line(const uint8_t row) const {
    if (row >= rows_ || row >= kHd44780SurfaceMaxRows) {
        return "";
    }
    return lines_[row].data();
}

} // namespace ewfm
