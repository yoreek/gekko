#pragma once

#include "devices/display/DisplayLayoutStore.h"

#include <vector>

namespace ewfm {

// Loads (once) and caches a Bitmap widget's raw pixel bytes from the default blob store into
// `widget.cachedImageBytes`, keyed by `widget.imageKey`. Same "compute once against a const widget
// reference, mutable field does the caching" shape as ensureDisplayTextWidgetAst
// (DisplayTextPlaceholderAst.h) - reset happens for free on the next layout load, since that always
// constructs a fresh DisplayLayoutWidgetV1.
//
// Returns nullptr if the widget has no imageKey, the blob store is unavailable, or no blob exists
// for that key - callers should treat any of those as "nothing to draw", not an error, since a
// dangling key can't reach the renderer for a persisted config (validateLayoutImageKeys rejects it
// at save time) but could still happen transiently (e.g. blob deleted out from under a live
// config).
const std::vector<uint8_t>* ensureDisplayBitmapBytes(const DisplayLayoutWidgetV1& widget);

} // namespace ewfm
