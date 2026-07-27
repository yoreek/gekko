#pragma once

#include "devices/display/DisplayDigitalFormatter.h"
#include "devices/display/DisplayLayoutStore.h"
#include "devices/display/DisplayTextEvaluator.h"
#include "devices/display/DisplayTextPlaceholderAst.h"

#include <cstdint>

namespace ewfm {

class IDisplayRenderSurface {
public:
    virtual ~IDisplayRenderSurface() = default;

    virtual void clear(uint16_t color) = 0;
    virtual void drawText(const DisplayLayoutWidgetV1& widget, const DisplayTextEvaluationResult& text) = 0;
    virtual void drawDigital(const DisplayLayoutWidgetV1& widget, const DisplayDigitalFrame& frame) {
        (void)widget;
        (void)frame;
    }
    virtual void drawRect(const DisplayLayoutWidgetV1& widget) = 0;
    virtual void drawLine(const DisplayLayoutWidgetV1& widget) = 0;
    virtual void drawCircle(const DisplayLayoutWidgetV1& widget) = 0;
    virtual void drawEllipse(const DisplayLayoutWidgetV1& widget) = 0;
    virtual void drawBitmap(const DisplayLayoutWidgetV1& widget) = 0;
};

struct DisplayLayoutRenderResult {
    bool rendered{false};
    bool cleared{false};
    bool pageChanged{false};
    bool hasDynamicWidgets{false};
    uint8_t activePageIndex{0};
    uint16_t refreshIntervalMs{kDisplayLayoutRefreshIntervalDisabled};
    uint8_t renderedWidgetCount{0};
};

class DisplayLayoutRenderSession {
public:
    void invalidate();

    DisplayLayoutRenderResult render(const DisplayLayoutRecordV1& layout, const MetricValueResolver& resolver,
                                     IDisplayRenderSurface& surface, uint32_t now);

private:
    static uint16_t widgetRefreshInterval(const DisplayLayoutWidgetV1& widget, bool dynamic);

    bool initialized_{false};
    uint8_t renderedPageIndex_{0xFFU};
    uint32_t nextRefreshAtMs_{0U};
    bool textSourcesBound_{false};
};

} // namespace ewfm
