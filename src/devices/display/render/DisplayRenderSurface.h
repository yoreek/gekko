#pragma once

#include "devices/display/DisplayDigitalFormatter.h"
#include "devices/display/DisplayLayoutStore.h"
#include "devices/display/DisplayTextEvaluator.h"

#include <cstdint>

namespace ewfm {

class IDisplayRenderSurface {
public:
    virtual ~IDisplayRenderSurface() = 0;
};

class IPixelDisplayRenderSurface : public IDisplayRenderSurface {
public:
    virtual void clear(uint16_t color) = 0;
    virtual void drawText(const DisplayLayoutWidgetV1& widget, const DisplayTextEvaluationResult& text) = 0;
    virtual void drawRect(const DisplayLayoutWidgetV1& widget) = 0;
    virtual void drawLine(const DisplayLayoutWidgetV1& widget) = 0;
    virtual void drawCircle(const DisplayLayoutWidgetV1& widget) = 0;
    virtual void drawEllipse(const DisplayLayoutWidgetV1& widget) = 0;
    virtual void drawBitmap(const DisplayLayoutWidgetV1& widget) = 0;
};

class ICharacterDisplayRenderSurface : public IDisplayRenderSurface {
public:
    virtual void clear(uint16_t color) = 0;
    virtual void drawText(const DisplayLayoutWidgetV1& widget, const DisplayTextEvaluationResult& text) = 0;
};

class ISegmentDisplayRenderSurface : public IDisplayRenderSurface {
public:
    virtual void clear(uint16_t color) = 0;
    virtual void drawDigital(const DisplayLayoutWidgetV1& widget, const DisplayDigitalFrame& frame) = 0;
};

} // namespace ewfm
