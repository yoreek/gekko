#pragma once

#include "devices/core/DeviceTypes.h"

#include <ArduinoJson.h>

namespace ewfm {

// Shared by every pixel-effect config that carries a `PixelColor` field (PixelEffectSolidDevice,
// PixelEffectAlertDevice, ...) so the `{r,g,b}` JSON shape and channel clamping stay in one place.
PixelColor parsePixelColorJson(const JsonObjectConst& input, const char* key, const PixelColor& fallback);
void writePixelColorJson(JsonObject output, const char* key, const PixelColor& color);

} // namespace ewfm
