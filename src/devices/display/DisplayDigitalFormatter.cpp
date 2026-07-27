#include "devices/display/DisplayDigitalFormatter.h"

#include <cstring>

namespace ewfm {
namespace {

constexpr uint8_t kDigitalMaxCells = 8U;

bool isAlignValid(const uint8_t align) {
    return align <= static_cast<uint8_t>(DisplayDigitalAlign::Right);
}

uint8_t visibleTextLength(const char* text) {
    if (text == nullptr) {
        return 0U;
    }
    uint8_t visible = 0U;
    for (const char* cursor = text; *cursor != '\0'; ++cursor) {
        if (*cursor != '.') {
            ++visible;
        }
    }
    return visible;
}

uint8_t patternLength(const char* text) {
    if (text == nullptr) {
        return 0U;
    }
    return static_cast<uint8_t>(std::strlen(text));
}

void fillPattern(DisplayDigitalCell* cells, uint8_t count, const char* pattern) {
    const uint8_t length = patternLength(pattern);
    if (length == 0U) {
        for (uint8_t index = 0U; index < count; ++index) {
            cells[index].glyph = '-';
            cells[index].decimalPoint = 0U;
        }
        return;
    }
    for (uint8_t index = 0U; index < count; ++index) {
        cells[index].glyph = pattern[index % length];
        cells[index].decimalPoint = 0U;
    }
}

uint8_t writeTextRightAligned(const char* text, DisplayDigitalCell* cells, uint8_t cellCount) {
    const uint8_t length = visibleTextLength(text);
    if (length == 0U) {
        return 0U;
    }

    const uint8_t start = length >= cellCount ? 0U : static_cast<uint8_t>(cellCount - length);
    uint8_t cellIndex = start;
    for (const char* cursor = text; *cursor != '\0' && cellIndex < cellCount; ++cursor) {
        if (*cursor == '.') {
            if (cellIndex > start) {
                cells[cellIndex - 1U].decimalPoint = 1U;
            }
            continue;
        }
        cells[cellIndex].glyph = *cursor;
        ++cellIndex;
    }
    return cellIndex;
}

void writeTextLeftAligned(const char* text, DisplayDigitalCell* cells, uint8_t cellCount) {
    uint8_t cellIndex = 0U;
    for (const char* cursor = text; *cursor != '\0' && cellIndex < cellCount; ++cursor) {
        if (*cursor == '.') {
            if (cellIndex > 0U) {
                cells[cellIndex - 1U].decimalPoint = 1U;
            }
            continue;
        }
        cells[cellIndex].glyph = *cursor;
        ++cellIndex;
    }
}

void writeTextCentered(const char* text, DisplayDigitalCell* cells, uint8_t cellCount) {
    const uint8_t length = visibleTextLength(text);
    if (length == 0U) {
        return;
    }
    const uint8_t start = length >= cellCount ? 0U : static_cast<uint8_t>((cellCount - length) / 2U);
    uint8_t cellIndex = start;
    for (const char* cursor = text; *cursor != '\0' && cellIndex < cellCount; ++cursor) {
        if (*cursor == '.') {
            if (cellIndex > start) {
                cells[cellIndex - 1U].decimalPoint = 1U;
            }
            continue;
        }
        cells[cellIndex].glyph = *cursor;
        ++cellIndex;
    }
}

} // namespace

bool buildDisplayDigitalFrame(const DisplayLayoutWidgetV1& widget, const DisplayTextEvaluationResult& text, DisplayDigitalFrame& frame) {
    frame = {};
    const uint8_t requestedCellCount = widget.width == 0U ? 1U : widget.width;
    frame.cellCount = requestedCellCount > kDigitalMaxCells ? kDigitalMaxCells : requestedCellCount;
    if (frame.cellCount == 0U) {
        return false;
    }

    const char* sourceText = text.available ? text.text : nullptr;
    const bool missingText = sourceText == nullptr || sourceText[0] == '\0';
    const uint8_t align = isAlignValid(widget.digitalAlign) ? widget.digitalAlign : static_cast<uint8_t>(DisplayDigitalAlign::Right);

    if (missingText) {
        fillPattern(frame.cells, frame.cellCount, widget.digitalMissing[0] != '\0' ? widget.digitalMissing : "----");
        return true;
    }

    const uint8_t visibleLength = visibleTextLength(sourceText);
    if (visibleLength > frame.cellCount) {
        fillPattern(frame.cells, frame.cellCount, widget.digitalOverflow[0] != '\0' ? widget.digitalOverflow : "----");
        frame.overflowed = true;
        return true;
    }

    switch (static_cast<DisplayDigitalAlign>(align)) {
    case DisplayDigitalAlign::Left:
        writeTextLeftAligned(sourceText, frame.cells, frame.cellCount);
        break;
    case DisplayDigitalAlign::Center:
        writeTextCentered(sourceText, frame.cells, frame.cellCount);
        break;
    case DisplayDigitalAlign::Right:
    default:
        (void)writeTextRightAligned(sourceText, frame.cells, frame.cellCount);
        break;
    }
    return true;
}

} // namespace ewfm
