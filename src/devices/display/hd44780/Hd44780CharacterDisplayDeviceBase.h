#pragma once

#include "devices/display/DisplayDeviceBase.h"
#include "devices/display/DisplayLayoutRenderer.h"
#include "devices/display/hd44780/Hd44780CharacterSurface.h"
#include "devices/display/render/CharacterDisplayLayoutRenderer.h"

#include <cstdint>

namespace ewfm {

constexpr uint8_t kHd44780MaxColumns = 20U;
constexpr uint8_t kHd44780MaxRows = 4U;

// Fixed logical line indices passed to setLine() - not dependency slots or hardware channel/pin
// numbers. The concrete transport (Hd44780I2cCharacterDisplayDeviceBase /
// Hd44780PinCharacterDisplayDeviceBase) maps each one to its own config field.
constexpr uint8_t kHd44780LineRs = 0U;
constexpr uint8_t kHd44780LineE = 1U;
constexpr uint8_t kHd44780LineD4 = 2U;
constexpr uint8_t kHd44780LineD5 = 3U;
constexpr uint8_t kHd44780LineD6 = 4U;
constexpr uint8_t kHd44780LineD7 = 5U;
constexpr uint8_t kHd44780LineBacklight = 6U;

// Shared runtime for HD44780 character displays. The derived leaf's transport (an embedded PCF8574
// I2C backpack or direct ESP32 GPIOs) supplies only setLine() and geometry; this base owns the
// HD44780 4-bit protocol timing, cooperative lifecycle, and the common display layout/rendering
// path. The layout is persisted separately from the hardware config.
class Hd44780CharacterDisplayDeviceBase : public DisplayDeviceBase {
public:
    static PState initialState();

    DisplayLayoutProfile displayProfile() const override;
    bool renderText(const MetricValueResolver& resolver, uint32_t now);
    const char* renderedLine(uint8_t row) const;

protected:
    Hd44780CharacterDisplayDeviceBase(PState initialState, uint8_t columns, uint8_t rows);

    // lineIndex is one of the kHd44780Line* constants above. Returns false if the transport
    // couldn't apply the level (I2C write failed, bus not ready, ...); callers abort the current
    // byte/line write and the next render tick retries rather than assuming success. An unwired
    // optional line (e.g. backlight) is a no-op that still returns true.
    virtual bool setLine(uint8_t lineIndex, bool level, uint32_t now) = 0;

    void resetRenderedLines();

    bool clearDisplay(uint16_t color) override;
    DisplayLayoutRenderResult renderDisplayFrame(const MetricValueResolver& resolver, uint32_t now) override;
    bool initializeDisplayHardware(uint32_t now) override;
    void releaseDisplayHardware(uint32_t now) override;
    void onDisplayFrameRendered(const DisplayLayoutRenderResult& result) override;

    State Idle();
    State Starting();
    State Ready();
    State Reconfiguring();
    State Disabled();
    State Faulted();
    State Deleting();

private:
    bool writeNibble(uint8_t nibble, bool rs, uint32_t now);
    bool writeByte(uint8_t value, bool rs, uint32_t now);
    bool runInitSequence(uint32_t now);
    bool writeLine(uint8_t row, const char* text, uint32_t now);
    uint8_t rowAddress(uint8_t row) const;

    uint8_t columns_;
    uint8_t rows_;
    Hd44780CharacterSurface renderSurface_;
    bool hasRenderedOnce_{false};
    char lastLines_[kHd44780MaxRows][kHd44780MaxColumns + 1U]{};
};

} // namespace ewfm
