#pragma once

#include "devices/display/DisplayDigitalFormatter.h"

#include <cstddef>
#include <cstdint>

namespace ewfm {

class Tm1637SegmentCodec final {
public:
    static constexpr uint8_t kDigitCount = 4U;

    static bool encode(const DisplayDigitalFrame& frame, uint8_t rotation, uint8_t* out, size_t outSize);

private:
    static uint8_t encodeGlyph(char glyph);
    static uint8_t rotateSegments180(uint8_t segments);
};

} // namespace ewfm
