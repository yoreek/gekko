#include "devices/display/DisplayLayoutValidator.h"

#include <cstring>

namespace ewfm {
namespace {

bool widgetTypeSupported(const DisplayLayoutProfile& profile, const DisplayLayoutWidgetType type) {
    return (profile.supportedWidgetMask & displayLayoutWidgetMask(type)) != 0U;
}

bool validateLogicalBounds(const DisplayLayoutWidgetV1& widget, const DisplayLayoutProfile& profile) {
    if (profile.logicalWidth == 0U && profile.logicalHeight == 0U) {
        return true;
    }
    if (widget.x >= profile.logicalWidth || widget.y >= profile.logicalHeight) {
        return false;
    }
    if (widget.x + widget.width > profile.logicalWidth || widget.y + widget.height > profile.logicalHeight) {
        return false;
    }
    return true;
}

} // namespace

DeviceValidationResult validateDisplayLayoutWidget(const DisplayLayoutWidgetV1& widget, const DisplayLayoutProfile& profile) {
    const DisplayLayoutWidgetType type = static_cast<DisplayLayoutWidgetType>(widget.type);
    if (!widgetTypeSupported(profile, type)) {
        return {DeviceError::InvalidConfig, "display widget type is not supported"};
    }
    if (widget.width == 0U || widget.height == 0U) {
        return {DeviceError::InvalidConfig, "display widget size is invalid"};
    }
    if (!validateLogicalBounds(widget, profile)) {
        return {DeviceError::InvalidConfig, "display widget bounds are invalid"};
    }
    if (type == DisplayLayoutWidgetType::Digital) {
        if (widget.height != 1U) {
            return {DeviceError::InvalidConfig, "display digital widget height must be 1"};
        }
        if (widget.digitalAlign > static_cast<uint8_t>(DisplayDigitalAlign::Right)) {
            return {DeviceError::InvalidConfig, "display digital align is invalid"};
        }
    }
    if (profile.coordinateUnit == DisplayCoordinateUnit::CharacterCell && type != DisplayLayoutWidgetType::Character) {
        return {DeviceError::InvalidConfig, "character display requires Character widgets"};
    }
    if (profile.coordinateUnit == DisplayCoordinateUnit::DigitCell && type != DisplayLayoutWidgetType::Digital) {
        return {DeviceError::InvalidConfig, "segment display requires Digital widgets"};
    }
    if (!profile.supportsColor && widget.color != 0xFFFFU) {
        return {DeviceError::InvalidConfig, "display widget color is not supported"};
    }
    if (type == DisplayLayoutWidgetType::Bitmap && !profile.supportsBitmap) {
        return {DeviceError::InvalidConfig, "bitmap widgets are not supported"};
    }
    if (std::strlen(widget.text) > profile.textCapacity) {
        return {DeviceError::InvalidConfig, "display widget text is too long"};
    }
    return {};
}

DeviceValidationResult validateDisplayLayout(const DisplayLayoutRecordV1& layout, const DisplayLayoutProfile& profile) {
    if (layout.pages.size() > profile.maxPages) {
        return {DeviceError::InvalidConfig, "display layout page count is invalid"};
    }
    if (layout.pages.empty()) {
        return {DeviceError::InvalidConfig, "display layout requires at least one page"};
    }
    if (layout.activePageIndex >= layout.pages.size()) {
        return {DeviceError::InvalidConfig, "display layout active page index is invalid"};
    }
    for (const DisplayLayoutPageV1& page : layout.pages) {
        if (page.widgets.size() > profile.maxWidgetsPerPage) {
            return {DeviceError::InvalidConfig, "display layout widget count is invalid"};
        }
        if (std::strlen(page.id) == 0U || std::strlen(page.id) > profile.pageIdCapacity) {
            return {DeviceError::InvalidConfig, "display layout page id is invalid"};
        }
        for (const DisplayLayoutWidgetV1& widget : page.widgets) {
            const DeviceValidationResult result = validateDisplayLayoutWidget(widget, profile);
            if (!result.ok()) {
                return result;
            }
        }
    }
    return {};
}

} // namespace ewfm
