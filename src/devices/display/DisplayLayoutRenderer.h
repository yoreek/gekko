#pragma once

#include "core/StateMachine.h"
#include "devices/display/DisplayDigitalFormatter.h"
#include "devices/display/DisplayLayoutStore.h"
#include "devices/display/DisplayTextEvaluator.h"
#include "devices/display/DisplayTextPlaceholderAst.h"
#include "devices/display/render/DisplayRenderSurface.h"

#include <cstdint>

namespace ewfm {

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

    template <typename Renderer>
    DisplayLayoutRenderResult render(const DisplayLayoutRecordV1& layout, const MetricValueResolver& resolver, Renderer& renderer,
                                     uint32_t now) {
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

        renderer.clear(layout.backgroundColor);
        result.cleared = true;

        uint16_t minimumRefreshIntervalMs = 0U;
        uint8_t renderedWidgetCount = 0U;
        bool hasDynamicWidgets = false;
        for (size_t widgetIndex = 0; widgetIndex < page->widgets.size(); ++widgetIndex) {
            const DisplayLayoutWidgetV1& widget = page->widgets[widgetIndex];
            const DisplayLayoutWidgetType type = static_cast<DisplayLayoutWidgetType>(widget.type);
            if (type == DisplayLayoutWidgetType::Text || type == DisplayLayoutWidgetType::Character) {
                if constexpr (Renderer::kSupportsText) {
                    bool dynamic = false;
                    renderTextWidget(widget, resolver, renderer, dynamic);
                    const uint16_t effectiveIntervalMs = widgetRefreshInterval(widget, dynamic);
                    if (effectiveIntervalMs != kDisplayLayoutRefreshIntervalDisabled &&
                        (minimumRefreshIntervalMs == 0U || effectiveIntervalMs < minimumRefreshIntervalMs)) {
                        minimumRefreshIntervalMs = effectiveIntervalMs;
                    }
                    hasDynamicWidgets = hasDynamicWidgets || dynamic;
                    ++renderedWidgetCount;
                }
                continue;
            }
            if (type == DisplayLayoutWidgetType::Digital) {
                if constexpr (Renderer::kSupportsDigital) {
                    bool dynamic = false;
                    renderDigitalWidget(widget, resolver, renderer, dynamic);
                    const uint16_t effectiveIntervalMs = widgetRefreshInterval(widget, dynamic);
                    if (effectiveIntervalMs != kDisplayLayoutRefreshIntervalDisabled &&
                        (minimumRefreshIntervalMs == 0U || effectiveIntervalMs < minimumRefreshIntervalMs)) {
                        minimumRefreshIntervalMs = effectiveIntervalMs;
                    }
                    hasDynamicWidgets = hasDynamicWidgets || dynamic;
                    ++renderedWidgetCount;
                }
                continue;
            }
            if (type == DisplayLayoutWidgetType::Bitmap) {
                if constexpr (Renderer::kSupportsBitmap) {
                    renderer.drawBitmap(widget);
                    ++renderedWidgetCount;
                }
                continue;
            }
            if (type == DisplayLayoutWidgetType::Rect) {
                if constexpr (Renderer::kSupportsShapes) {
                    renderer.drawRect(widget);
                    ++renderedWidgetCount;
                }
                continue;
            }
            if (type == DisplayLayoutWidgetType::Line) {
                if constexpr (Renderer::kSupportsShapes) {
                    renderer.drawLine(widget);
                    ++renderedWidgetCount;
                }
                continue;
            }
            if (type == DisplayLayoutWidgetType::Circle) {
                if constexpr (Renderer::kSupportsShapes) {
                    renderer.drawCircle(widget);
                    ++renderedWidgetCount;
                }
                continue;
            }
            if (type == DisplayLayoutWidgetType::Ellipse) {
                if constexpr (Renderer::kSupportsShapes) {
                    renderer.drawEllipse(widget);
                    ++renderedWidgetCount;
                }
                continue;
            }
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

private:
    static const DisplayLayoutPageV1* activePage(const DisplayLayoutRecordV1& layout);
    template <typename Renderer>
    static void renderTextWidget(const DisplayLayoutWidgetV1& widget, const MetricValueResolver& resolver, Renderer& renderer,
                                 bool& dynamic) {
        DisplayTextEvaluationResult text{};
        const DisplayTextCompiledWidget* compiled = nullptr;
        if (ensureDisplayTextWidgetAst(widget, compiled, nullptr) && compiled != nullptr) {
            (void)evaluateDisplayTextWidget(widget.text, *compiled, resolver, text);
        } else {
            (void)evaluateDisplayTextWidget(widget, resolver, text);
        }
        dynamic = text.dynamic;
        renderer.drawText(widget, text);
    }

    template <typename Renderer>
    static void renderDigitalWidget(const DisplayLayoutWidgetV1& widget, const MetricValueResolver& resolver, Renderer& renderer,
                                    bool& dynamic) {
        DisplayTextEvaluationResult text{};
        const DisplayTextCompiledWidget* compiled = nullptr;
        if (ensureDisplayTextWidgetAst(widget, compiled, nullptr) && compiled != nullptr) {
            (void)evaluateDisplayTextWidget(widget.text, *compiled, resolver, text);
        } else {
            (void)evaluateDisplayTextWidget(widget, resolver, text);
        }
        dynamic = text.dynamic;

        DisplayDigitalFrame frame{};
        if (!buildDisplayDigitalFrame(widget, text, frame)) {
            return;
        }
        renderer.drawDigital(widget, frame);
    }

    static uint16_t widgetRefreshInterval(const DisplayLayoutWidgetV1& widget, bool dynamic);

    bool initialized_{false};
    uint8_t renderedPageIndex_{0xFFU};
    uint32_t nextRefreshAtMs_{0U};
    bool textSourcesBound_{false};
};

} // namespace ewfm
