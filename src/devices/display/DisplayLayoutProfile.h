#pragma once

#include "devices/display/DisplayLayoutStore.h"

#include <ArduinoJson.h>
#include <cstdint>

namespace ewfm {

enum class DisplayCoordinateUnit : uint8_t {
    Pixel = 0,
    CharacterCell = 1,
    DigitCell = 2,
};

enum class DisplayAuxSegmentMode : uint8_t {
    None = 0,
    SharedColon = 1,
    PerDigitDecimalPoint = 2,
};

struct DisplayLayoutProfile {
    DisplayCoordinateUnit coordinateUnit{DisplayCoordinateUnit::Pixel};
    uint8_t logicalWidth{0U};
    uint8_t logicalHeight{0U};
    uint32_t supportedWidgetMask{0U};
    uint8_t supportedRotationsMask{0x0FU};
    uint8_t maxPages{kDisplayLayoutMaxPages};
    uint8_t maxWidgetsPerPage{kDisplayLayoutMaxWidgetsPerPage};
    uint8_t pageIdCapacity{static_cast<uint8_t>(kDisplayLayoutPageIdCapacity)};
    uint8_t textCapacity{static_cast<uint8_t>(kDisplayLayoutTextCapacity)};
    size_t maxBitmapBytes{kDisplayLayoutBitmapDataCapacity};
    bool supportsColor{true};
    bool supportsBitmap{true};
    DisplayAuxSegmentMode auxSegmentMode{DisplayAuxSegmentMode::None};
};

constexpr uint32_t displayLayoutWidgetMask(DisplayLayoutWidgetType type) {
    return 1UL << static_cast<uint8_t>(type);
}

constexpr DisplayLayoutProfile makeDisplayLayoutProfile(DisplayCoordinateUnit coordinateUnit, uint8_t logicalWidth, uint8_t logicalHeight,
                                                        uint32_t supportedWidgetMask, uint8_t supportedRotationsMask = 0x0FU,
                                                        uint8_t maxPages = static_cast<uint8_t>(kDisplayLayoutMaxPages),
                                                        uint8_t maxWidgetsPerPage = static_cast<uint8_t>(kDisplayLayoutMaxWidgetsPerPage),
                                                        uint8_t pageIdCapacity = static_cast<uint8_t>(kDisplayLayoutPageIdCapacity),
                                                        uint8_t textCapacity = static_cast<uint8_t>(kDisplayLayoutTextCapacity),
                                                        size_t maxBitmapBytes = kDisplayLayoutBitmapDataCapacity, bool supportsColor = true,
                                                        bool supportsBitmap = true,
                                                        DisplayAuxSegmentMode auxSegmentMode = DisplayAuxSegmentMode::None) {
    return {
        coordinateUnit, logicalWidth, logicalHeight,  supportedWidgetMask, supportedRotationsMask, maxPages,       maxWidgetsPerPage,
        pageIdCapacity, textCapacity, maxBitmapBytes, supportsColor,       supportsBitmap,         auxSegmentMode,
    };
}

inline constexpr DisplayLayoutProfile defaultDisplayLayoutProfile() {
    return makeDisplayLayoutProfile(
        DisplayCoordinateUnit::Pixel, 0U, 0U,
        displayLayoutWidgetMask(DisplayLayoutWidgetType::Text) | displayLayoutWidgetMask(DisplayLayoutWidgetType::Digital) |
            displayLayoutWidgetMask(DisplayLayoutWidgetType::Bitmap) | displayLayoutWidgetMask(DisplayLayoutWidgetType::Rect) |
            displayLayoutWidgetMask(DisplayLayoutWidgetType::Line) | displayLayoutWidgetMask(DisplayLayoutWidgetType::Circle) |
            displayLayoutWidgetMask(DisplayLayoutWidgetType::Ellipse) | displayLayoutWidgetMask(DisplayLayoutWidgetType::Character));
}

inline constexpr DisplayLayoutProfile pixelDisplayLayoutProfile(uint8_t logicalWidth, uint8_t logicalHeight, bool supportsColor,
                                                                bool supportsBitmap, uint8_t supportedRotationsMask = 0x0FU) {
    return makeDisplayLayoutProfile(
        DisplayCoordinateUnit::Pixel, logicalWidth, logicalHeight,
        displayLayoutWidgetMask(DisplayLayoutWidgetType::Text) | displayLayoutWidgetMask(DisplayLayoutWidgetType::Bitmap) |
            displayLayoutWidgetMask(DisplayLayoutWidgetType::Rect) | displayLayoutWidgetMask(DisplayLayoutWidgetType::Line) |
            displayLayoutWidgetMask(DisplayLayoutWidgetType::Circle) | displayLayoutWidgetMask(DisplayLayoutWidgetType::Ellipse),
        supportedRotationsMask, static_cast<uint8_t>(kDisplayLayoutMaxPages), static_cast<uint8_t>(kDisplayLayoutMaxWidgetsPerPage),
        static_cast<uint8_t>(kDisplayLayoutPageIdCapacity), static_cast<uint8_t>(kDisplayLayoutTextCapacity),
        kDisplayLayoutBitmapDataCapacity, supportsColor, supportsBitmap);
}

inline constexpr DisplayLayoutProfile characterCellDisplayLayoutProfile(uint8_t columns, uint8_t rows,
                                                                        uint8_t supportedRotationsMask = 0x01U) {
    return makeDisplayLayoutProfile(
        DisplayCoordinateUnit::CharacterCell, columns, rows, displayLayoutWidgetMask(DisplayLayoutWidgetType::Character),
        supportedRotationsMask, static_cast<uint8_t>(kDisplayLayoutMaxPages), static_cast<uint8_t>(kDisplayLayoutMaxWidgetsPerPage),
        static_cast<uint8_t>(kDisplayLayoutPageIdCapacity), static_cast<uint8_t>(kDisplayLayoutTextCapacity), 0U, false, false);
}

inline constexpr DisplayLayoutProfile digitCellDisplayLayoutProfile(uint8_t digits, DisplayAuxSegmentMode auxSegmentMode,
                                                                    uint8_t supportedRotationsMask = 0x05U) {
    return makeDisplayLayoutProfile(DisplayCoordinateUnit::DigitCell, digits, 1U, displayLayoutWidgetMask(DisplayLayoutWidgetType::Digital),
                                    supportedRotationsMask, static_cast<uint8_t>(kDisplayLayoutMaxPages),
                                    static_cast<uint8_t>(kDisplayLayoutMaxWidgetsPerPage),
                                    static_cast<uint8_t>(kDisplayLayoutPageIdCapacity), static_cast<uint8_t>(kDisplayLayoutTextCapacity),
                                    0U, false, false, auxSegmentMode);
}

inline constexpr DisplayLayoutProfile segmentDisplayLayoutProfile(uint8_t digits, uint8_t supportedRotationsMask = 0x05U) {
    return digitCellDisplayLayoutProfile(digits, DisplayAuxSegmentMode::PerDigitDecimalPoint, supportedRotationsMask);
}

inline void writeDisplayLayoutProfileJson(const DisplayLayoutProfile& profile, JsonObject output) {
    output["coordinateUnit"] = static_cast<uint8_t>(profile.coordinateUnit);
    output["logicalWidth"] = profile.logicalWidth;
    output["logicalHeight"] = profile.logicalHeight;
    output["supportedWidgetMask"] = profile.supportedWidgetMask;
    output["supportedRotationsMask"] = profile.supportedRotationsMask;
    output["maxPages"] = profile.maxPages;
    output["maxWidgetsPerPage"] = profile.maxWidgetsPerPage;
    output["pageIdCapacity"] = profile.pageIdCapacity;
    output["textCapacity"] = profile.textCapacity;
    output["maxBitmapBytes"] = profile.maxBitmapBytes;
    output["supportsColor"] = profile.supportsColor;
    output["supportsBitmap"] = profile.supportsBitmap;
    output["auxSegmentMode"] = static_cast<uint8_t>(profile.auxSegmentMode);
}

} // namespace ewfm
