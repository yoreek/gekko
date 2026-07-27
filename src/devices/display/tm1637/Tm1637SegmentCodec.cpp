#include "devices/display/tm1637/Tm1637SegmentCodec.h"

#include <cstring>

namespace ewfm {
namespace {
constexpr uint8_t kSegmentDp = 0x80U;
constexpr uint8_t kSegmentA = 0x01U;
constexpr uint8_t kSegmentB = 0x02U;
constexpr uint8_t kSegmentC = 0x04U;
constexpr uint8_t kSegmentD = 0x08U;
constexpr uint8_t kSegmentE = 0x10U;
constexpr uint8_t kSegmentF = 0x20U;
constexpr uint8_t kSegmentG = 0x40U;
} // namespace

uint8_t Tm1637SegmentCodec::encodeGlyph(const char glyph) {
    switch (glyph) {
    case '0':
        return static_cast<uint8_t>(kSegmentA | kSegmentB | kSegmentC | kSegmentD | kSegmentE | kSegmentF);
    case '1':
        return static_cast<uint8_t>(kSegmentB | kSegmentC);
    case '2':
        return static_cast<uint8_t>(kSegmentA | kSegmentB | kSegmentD | kSegmentE | kSegmentG);
    case '3':
        return static_cast<uint8_t>(kSegmentA | kSegmentB | kSegmentC | kSegmentD | kSegmentG);
    case '4':
        return static_cast<uint8_t>(kSegmentB | kSegmentC | kSegmentF | kSegmentG);
    case '5':
        return static_cast<uint8_t>(kSegmentA | kSegmentC | kSegmentD | kSegmentF | kSegmentG);
    case '6':
        return static_cast<uint8_t>(kSegmentA | kSegmentC | kSegmentD | kSegmentE | kSegmentF | kSegmentG);
    case '7':
        return static_cast<uint8_t>(kSegmentA | kSegmentB | kSegmentC);
    case '8':
        return static_cast<uint8_t>(kSegmentA | kSegmentB | kSegmentC | kSegmentD | kSegmentE | kSegmentF | kSegmentG);
    case '9':
        return static_cast<uint8_t>(kSegmentA | kSegmentB | kSegmentC | kSegmentD | kSegmentF | kSegmentG);
    case '-':
        return kSegmentG;
    case 'A':
    case 'a':
        return static_cast<uint8_t>(kSegmentA | kSegmentB | kSegmentC | kSegmentE | kSegmentF | kSegmentG);
    case 'b':
        return static_cast<uint8_t>(kSegmentC | kSegmentD | kSegmentE | kSegmentF | kSegmentG);
    case 'C':
    case 'c':
        return static_cast<uint8_t>(kSegmentA | kSegmentD | kSegmentE | kSegmentF);
    case 'd':
        return static_cast<uint8_t>(kSegmentB | kSegmentC | kSegmentD | kSegmentE | kSegmentG);
    case 'E':
    case 'e':
        return static_cast<uint8_t>(kSegmentA | kSegmentD | kSegmentE | kSegmentF | kSegmentG);
    case 'F':
    case 'f':
        return static_cast<uint8_t>(kSegmentA | kSegmentE | kSegmentF | kSegmentG);
    case 'H':
    case 'h':
        return static_cast<uint8_t>(kSegmentB | kSegmentC | kSegmentE | kSegmentF | kSegmentG);
    case 'L':
    case 'l':
        return static_cast<uint8_t>(kSegmentD | kSegmentE | kSegmentF);
    case 'P':
    case 'p':
        return static_cast<uint8_t>(kSegmentA | kSegmentB | kSegmentE | kSegmentF | kSegmentG);
    case 'U':
    case 'u':
        return static_cast<uint8_t>(kSegmentB | kSegmentC | kSegmentD | kSegmentE | kSegmentF);
    case ' ':
    default:
        return 0U;
    }
}

uint8_t Tm1637SegmentCodec::rotateSegments180(const uint8_t segments) {
    uint8_t rotated = 0U;
    if ((segments & kSegmentA) != 0U) {
        rotated |= kSegmentD;
    }
    if ((segments & kSegmentB) != 0U) {
        rotated |= kSegmentE;
    }
    if ((segments & kSegmentC) != 0U) {
        rotated |= kSegmentF;
    }
    if ((segments & kSegmentD) != 0U) {
        rotated |= kSegmentA;
    }
    if ((segments & kSegmentE) != 0U) {
        rotated |= kSegmentB;
    }
    if ((segments & kSegmentF) != 0U) {
        rotated |= kSegmentC;
    }
    if ((segments & kSegmentG) != 0U) {
        rotated |= kSegmentG;
    }
    if ((segments & kSegmentDp) != 0U) {
        rotated |= kSegmentDp;
    }
    return rotated;
}

bool Tm1637SegmentCodec::encode(const DisplayDigitalFrame& frame, const uint8_t rotation, uint8_t* out, const size_t outSize) {
    if (out == nullptr || outSize < kDigitCount) {
        return false;
    }
    for (uint8_t digit = 0U; digit < kDigitCount; ++digit) {
        uint8_t value = 0U;
        if (digit < frame.cellCount) {
            const DisplayDigitalCell cell = frame.cells[digit];
            value = encodeGlyph(cell.glyph);
            if (cell.decimalPoint != 0U) {
                value = static_cast<uint8_t>(value | kSegmentDp);
            }
        }
        const uint8_t outputDigit = rotation == 180U ? static_cast<uint8_t>(kDigitCount - 1U - digit) : digit;
        out[outputDigit] = rotation == 180U ? rotateSegments180(value) : value;
    }
    return true;
}

} // namespace ewfm
