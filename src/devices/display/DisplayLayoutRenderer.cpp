#include "devices/display/DisplayLayoutRenderer.h"

#include "core/StateMachine.h"

namespace ewfm {

namespace {

const DisplayLayoutPageV1* activePage(const DisplayLayoutRecordV1& layout) {
    if (layout.pages.empty() || layout.activePageIndex >= layout.pages.size()) {
        return nullptr;
    }
    return &layout.pages[layout.activePageIndex];
}

void renderTextWidget(const DisplayLayoutWidgetV1& widget, const MetricValueResolver& resolver, IDisplayRenderSurface& surface,
                      bool& dynamic) {
    DisplayTextEvaluationResult text{};
    const DisplayTextCompiledWidget* compiled = nullptr;
    if (ensureDisplayTextWidgetAst(widget, compiled, nullptr) && compiled != nullptr) {
        (void)evaluateDisplayTextWidget(widget.text, *compiled, resolver, text);
    } else {
        (void)evaluateDisplayTextWidget(widget, resolver, text);
    }
    dynamic = text.dynamic;
    surface.drawText(widget, text);
}

} // namespace

void DisplayLayoutRenderSession::invalidate() {
    initialized_ = false;
    renderedPageIndex_ = 0xFFU;
    nextRefreshAtMs_ = 0U;
    textSourcesBound_ = false;
}

uint16_t DisplayLayoutRenderSession::widgetRefreshInterval(const DisplayLayoutWidgetV1& widget, const bool dynamic) {
    if (!dynamic) {
        return kDisplayLayoutRefreshIntervalDisabled;
    }
    if (widget.refreshIntervalMs == kDisplayLayoutRefreshIntervalDisabled) {
        return kDisplayLayoutRefreshIntervalMinMs;
    }
    return widget.refreshIntervalMs;
}

DisplayLayoutRenderResult DisplayLayoutRenderSession::render(const DisplayLayoutRecordV1& layout, const MetricValueResolver& resolver,
                                                             IDisplayRenderSurface& surface, const uint32_t now) {
    DisplayLayoutRenderResult result{};
    result.activePageIndex = layout.activePageIndex;

    const DisplayLayoutPageV1* page = activePage(layout);
    if (page == nullptr) {
        return result;
    }

    if (!textSourcesBound_) {
        const DeviceRegistry* registry = resolver.registry();
        if (registry != nullptr && bindDisplayLayoutTextAst(layout, *registry)) {
            textSourcesBound_ = true;
        }
    }

    const bool pageChanged = !initialized_ || renderedPageIndex_ != layout.activePageIndex;
    const bool refreshDue = pageChanged || !initialized_ || (nextRefreshAtMs_ != 0U && EWFM_SM_TIME_REACHED(now, nextRefreshAtMs_));
    result.pageChanged = pageChanged;
    if (!refreshDue) {
        return result;
    }

    surface.clear(layout.backgroundColor);
    result.cleared = true;

    uint16_t minimumRefreshIntervalMs = 0U;
    uint8_t renderedWidgetCount = 0U;
    bool hasDynamicWidgets = false;
    for (size_t widgetIndex = 0; widgetIndex < page->widgets.size(); ++widgetIndex) {
        const DisplayLayoutWidgetV1& widget = page->widgets[widgetIndex];
        const DisplayLayoutWidgetType type = static_cast<DisplayLayoutWidgetType>(widget.type);
        if (type == DisplayLayoutWidgetType::Text) {
            bool dynamic = false;
            renderTextWidget(widget, resolver, surface, dynamic);
            const uint16_t effectiveIntervalMs = widgetRefreshInterval(widget, dynamic);
            if (effectiveIntervalMs != kDisplayLayoutRefreshIntervalDisabled &&
                (minimumRefreshIntervalMs == 0U || effectiveIntervalMs < minimumRefreshIntervalMs)) {
                minimumRefreshIntervalMs = effectiveIntervalMs;
            }
            hasDynamicWidgets = hasDynamicWidgets || dynamic;
        } else if (type == DisplayLayoutWidgetType::Bitmap) {
            surface.drawBitmap(widget);
        } else if (type == DisplayLayoutWidgetType::Rect) {
            surface.drawRect(widget);
        } else if (type == DisplayLayoutWidgetType::Line) {
            surface.drawLine(widget);
        } else if (type == DisplayLayoutWidgetType::Circle) {
            surface.drawCircle(widget);
        } else if (type == DisplayLayoutWidgetType::Ellipse) {
            surface.drawEllipse(widget);
        } else {
            continue;
        }
        ++renderedWidgetCount;
    }

    initialized_ = true;
    renderedPageIndex_ = layout.activePageIndex;
    nextRefreshAtMs_ = minimumRefreshIntervalMs != 0U ? now + minimumRefreshIntervalMs : 0U;

    result.rendered = true;
    result.hasDynamicWidgets = hasDynamicWidgets;
    result.refreshIntervalMs = minimumRefreshIntervalMs;
    result.renderedWidgetCount = renderedWidgetCount;
    return result;
}

} // namespace ewfm
