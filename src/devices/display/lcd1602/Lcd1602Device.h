#pragma once

#include "devices/core/DeviceRuntimeBase.h"
#include "devices/display/CharacterDisplayRuntimeBase.h"
#include "devices/display/lcd1602/Lcd1602DeviceConfig.h"

#include <cstdint>
#include <memory>

namespace ewfm {

// Drives a 16x2 HD44780 character display in 4-bit mode entirely through an already-registered
// PCF8574/PCF8575 port-expander device's IPortExpanderRuntime (a DeviceRole::PortExpander
// dependency, the same way PortExpanderSwitchDevice reaches its expander) -- each
// requestChannelState() call is a synchronous, blocking, full-byte I2C write
// (Pcf857xExpanderDeviceBase::writeCurrentStates), so sequential calls give the ordered,
// byte-atomic register writes HD44780 needs, with no separate I2C driver of its own.
//
// This is deliberately *not* a DisplayDeviceBase descendant: HD44780 is a fixed 16x2 character
// grid with no pixel addressing, so the pixel-widget/page layout model (rect/circle/bitmap, x/y in
// pixels) doesn't apply. Content is two plain line templates reusing the same dev.<id>.<metricKey>
// placeholder support the pixel-display family uses (DisplayTextPlaceholderAst), rendered on
// request by DisplayRenderCoordinator through the CharacterDisplayRuntimeBase hook -- this class's
// own tick/state-machine only owns hardware lifecycle (running the HD44780 init sequence once
// dependencies are ready).
class Lcd1602Device final : public DeviceRuntimeBase, public CharacterDisplayRuntimeBase {
public:
    Lcd1602Device(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);
    explicit Lcd1602Device(const Lcd1602DeviceConfigV1& config);

    const Lcd1602DeviceConfigV1& config() const;
    const char* renderedLine1() const;
    const char* renderedLine2() const;
    bool serializeConfigBlob(DeviceConfigBlob& configBlob) const override;
    DeviceConfigUpdatePlan planConfigUpdate(const DeviceConfigBlob& configBlob) const override;
    bool applyConfig(const DeviceConfigBlob& configBlob, uint32_t now) override;
    uint8_t expanderChannels(uint8_t* out, uint8_t maxOut) const override;
    CharacterDisplayRuntimeBase* characterDisplayRuntime() override;
    bool renderText(const MetricValueResolver& resolver, uint32_t now) override;

    static DeviceTypeDescriptor descriptor();
    static std::unique_ptr<IDeviceRuntime> createRuntime(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);
    static DeviceValidationResult validateConfig(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);

private:
    const DeviceBaseConfigV1& baseConfig() const override;

    State Idle();
    State Starting();
    State Ready();
    State Reconfiguring();
    State Disabled();
    State Faulted();
    State Deleting();

    IPortExpanderRuntime* dependencyExpander() const;
    bool writeNibble(IPortExpanderRuntime& expander, uint8_t nibble, bool rs, uint32_t now) const;
    bool writeByte(IPortExpanderRuntime& expander, uint8_t value, bool rs, uint32_t now) const;
    bool runInitSequence(uint32_t now);
    bool writeLine(IPortExpanderRuntime& expander, uint8_t row, const char* text, uint32_t now) const;
    void resolveLine(const char* templateText, const MetricValueResolver& resolver, char (&out)[kLcd1602LineLength + 1U]) const;

    Lcd1602DeviceConfigV1 config_{};
    bool hasRenderedOnce_{false};
    char lastLine1_[kLcd1602LineLength + 1U]{};
    char lastLine2_[kLcd1602LineLength + 1U]{};
};

} // namespace ewfm
