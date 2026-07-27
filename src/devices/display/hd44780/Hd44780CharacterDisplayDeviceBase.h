#pragma once

#include "devices/display/DisplayDeviceBase.h"
#include "devices/display/DisplayLayoutRenderer.h"
#include "devices/display/hd44780/Hd44780ChannelConfig.h"
#include "devices/display/hd44780/Hd44780CharacterSurface.h"
#include "devices/display/render/CharacterDisplayLayoutRenderer.h"

#include <cstdint>

namespace ewfm {

constexpr uint8_t kHd44780MaxColumns = 20U;
constexpr uint8_t kHd44780MaxRows = 4U;

// Shared runtime for HD44780 character displays driven through per-slot Switch dependencies.
// The derived leaf supplies only geometry and dependency-slot wiring. Rendering goes through the
// common display layout path and the layout is persisted separately from the hardware config.
class Hd44780CharacterDisplayDeviceBase : public DisplayDeviceBase {
public:
    static PState initialState();

    uint8_t dependencySlots(uint8_t* out, uint8_t maxOut) const override;
    DisplayLayoutProfile displayProfile() const override;
    bool renderText(const MetricValueResolver& resolver, uint32_t now);
    const char* renderedLine(uint8_t row) const;

protected:
    Hd44780CharacterDisplayDeviceBase(PState initialState, uint8_t columns, uint8_t rows);

    virtual const Hd44780ChannelConfigV1& channelConfig() const = 0;

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
    ISwitchOutputRuntime* dependencySwitch(uint8_t slot) const;
    bool writeSwitchSlot(uint8_t slot, bool on, uint32_t now) const;
    bool writeNibble(uint8_t rsSlot, uint8_t dataSlot0, uint8_t dataSlot1, uint8_t dataSlot2, uint8_t dataSlot3, uint8_t eSlot,
                     uint8_t nibble, bool rs, uint32_t now) const;
    bool writeByte(uint8_t rsSlot, uint8_t dataSlot0, uint8_t dataSlot1, uint8_t dataSlot2, uint8_t dataSlot3, uint8_t eSlot, uint8_t value,
                   bool rs, uint32_t now) const;
    bool runInitSequence(uint32_t now);
    bool writeLine(uint8_t rsSlot, uint8_t dataSlot0, uint8_t dataSlot1, uint8_t dataSlot2, uint8_t dataSlot3, uint8_t eSlot, uint8_t row,
                   const char* text, uint32_t now) const;
    uint8_t rowAddress(uint8_t row) const;

    uint8_t columns_;
    uint8_t rows_;
    Hd44780CharacterSurface renderSurface_;
    bool hasRenderedOnce_{false};
    char lastLines_[kHd44780MaxRows][kHd44780MaxColumns + 1U]{};
};

} // namespace ewfm
