#include "devices/display/DisplayLayoutRenderer.h"

namespace ewfm {

namespace {} // namespace

const DisplayLayoutPageV1* DisplayLayoutRenderSession::activePage(const DisplayLayoutRecordV1& layout) {
    if (layout.pages.empty() || layout.activePageIndex >= layout.pages.size()) {
        return nullptr;
    }
    return &layout.pages[layout.activePageIndex];
}

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

} // namespace ewfm
