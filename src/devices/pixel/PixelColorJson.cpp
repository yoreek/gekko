#include "devices/pixel/PixelColorJson.h"

namespace ewfm {

namespace {
uint8_t parseColorChannel(const JsonObjectConst& colorInput, const char* channelKey, uint8_t fallback) {
    const JsonVariantConst field = colorInput[channelKey];
    if (field.isNull()) {
        return fallback;
    }
    const long value = field.as<long>();
    if (value < 0) {
        return 0U;
    }
    if (value > 255) {
        return 255U;
    }
    return static_cast<uint8_t>(value);
}
} // namespace

PixelColor parsePixelColorJson(const JsonObjectConst& input, const char* key, const PixelColor& fallback) {
    const JsonVariantConst colorField = input[key];
    if (!colorField.is<JsonObjectConst>()) {
        return fallback;
    }
    const JsonObjectConst colorInput = colorField.as<JsonObjectConst>();
    PixelColor color{};
    color.r = parseColorChannel(colorInput, "r", fallback.r);
    color.g = parseColorChannel(colorInput, "g", fallback.g);
    color.b = parseColorChannel(colorInput, "b", fallback.b);
    return color;
}

void writePixelColorJson(JsonObject output, const char* key, const PixelColor& color) {
    JsonObject colorJson = output.createNestedObject(key);
    colorJson["r"] = color.r;
    colorJson["g"] = color.g;
    colorJson["b"] = color.b;
}

} // namespace ewfm
