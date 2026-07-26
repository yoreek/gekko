#pragma once

#include "devices/core/DeviceRuntimeBase.h"
#include "devices/display/CharacterDisplayRuntimeBase.h"
#include "devices/display/hd44780/Hd44780ChannelConfig.h"

#include <cstdint>

namespace ewfm {

constexpr uint8_t kHd44780MaxColumns = 20U;
constexpr uint8_t kHd44780MaxRows = 4U;

// Shared runtime for any HD44780 character display driven in 4-bit mode entirely through an
// already-registered PCF8574/PCF8575 port-expander device's IPortExpanderRuntime (a
// DeviceRole::PortExpander dependency, the same way PortExpanderSwitchDevice reaches its expander)
// -- each requestChannelState() call is a synchronous, blocking, full-byte I2C write
// (Pcf857xExpanderDeviceBase::writeCurrentStates), so sequential calls give the ordered,
// byte-atomic register writes HD44780 needs, with no separate I2C driver of its own.
//
// This is deliberately *not* a DisplayDeviceBase descendant: HD44780 is a fixed character grid with
// no pixel addressing, so the pixel-widget/page layout model (rect/circle/bitmap, x/y in pixels)
// doesn't apply. Content is plain line templates reusing the same dev.<id>.<metricKey> placeholder
// support the pixel-display family uses (DisplayTextPlaceholderAst), rendered on request by
// DisplayRenderCoordinator through the CharacterDisplayRuntimeBase hook -- this class's own
// tick/state-machine only owns hardware lifecycle (running the HD44780 init sequence once
// dependencies are ready).
//
// Owns the protocol/lifecycle/rendering in full; a concrete leaf (Lcd1602Device, Lcd2004Device, ...)
// supplies only its geometry (columns/rows, fixed at construction) and line-template storage via
// the channelConfig()/lineTemplate() hooks -- mirrors how DisplayDeviceBase owns the pixel-display
// lifecycle while Ssd1306Device/St7735Device supply only hardware specifics.
//
// Row DDRAM addressing follows the general HD44780 rule, not just the well-known 20x4 case: rows
// 0/1 start at 0x00/0x40; rows 2/3 continue in the *same* 40-byte DDRAM half as rows 0/1, offset by
// `columns` (the controller only has two 40-byte halves regardless of visible width) --
// addr(row) = (row odd ? 0x40 : 0x00) + (row >= 2 ? columns : 0).
class Hd44780CharacterDisplayDeviceBase : public DeviceRuntimeBase, public CharacterDisplayRuntimeBase {
public:
    static PState initialState();

    uint8_t expanderChannels(uint8_t* out, uint8_t maxOut) const override;
    CharacterDisplayRuntimeBase* characterDisplayRuntime() override;
    bool renderText(const MetricValueResolver& resolver, uint32_t now) override;
    const char* renderedLine(uint8_t row) const;

protected:
    Hd44780CharacterDisplayDeviceBase(PState initialState, uint8_t columns, uint8_t rows);

    // Concrete leaf hooks: the channel wiring and the row's current text template (from its own
    // config), respectively. `row` is in [0, rows) as passed to the constructor.
    virtual const Hd44780ChannelConfigV1& channelConfig() const = 0;
    virtual const char* lineTemplate(uint8_t row) const = 0;

    void resetRenderedLines();

    State Idle();
    State Starting();
    State Ready();
    State Reconfiguring();
    State Disabled();
    State Faulted();
    State Deleting();

private:
    IPortExpanderRuntime* dependencyExpander() const;
    bool writeNibble(IPortExpanderRuntime& expander, uint8_t nibble, bool rs, uint32_t now) const;
    bool writeByte(IPortExpanderRuntime& expander, uint8_t value, bool rs, uint32_t now) const;
    bool runInitSequence(uint32_t now);
    bool writeLine(IPortExpanderRuntime& expander, uint8_t row, const char* text, uint32_t now) const;
    void resolveLine(const char* templateText, const MetricValueResolver& resolver, char* out, uint8_t capacity) const;
    uint8_t rowAddress(uint8_t row) const;

    uint8_t columns_;
    uint8_t rows_;
    bool hasRenderedOnce_{false};
    char lastLines_[kHd44780MaxRows][kHd44780MaxColumns + 1U]{};
};

} // namespace ewfm
