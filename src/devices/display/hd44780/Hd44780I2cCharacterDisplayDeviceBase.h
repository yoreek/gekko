#pragma once

#include "devices/bus/i2c/I2cBusDevice.h"
#include "devices/display/hd44780/Hd44780CharacterDisplayDeviceBase.h"
#include "devices/display/hd44780/Hd44780DisplayDeviceConfigBase.h"
#include "devices/expander/Pcf857xIoDriver.h"

namespace ewfm {

// Read-only snapshot of the fields a concrete leaf's I2cDeviceConfigV1-derived config carries for
// wiring: the PCF8574 bit position for each HD44780 line, plus the chip's I2C address. A plain
// value type (not a reference into the config) so the leaf's own struct shape never has to match
// this class's layout.
struct Hd44780I2cLineChannels {
    uint8_t i2cAddress;
    uint8_t rsChannel;
    uint8_t eChannel;
    uint8_t d4Channel;
    uint8_t d5Channel;
    uint8_t d6Channel;
    uint8_t d7Channel;
    uint8_t backlightChannel; // kHd44780ChannelUnset = not wired
};

// I2C transport for HD44780 displays wired through an embedded PCF8574 backpack. Talks straight to
// the bus dependency (dependencyRuntime(DeviceRole::I2CBus) -> I2cBusDevice::DependencyTransaction,
// mirroring Ssd1306Device::initializeDisplayHardware) using Pcf857xIoDriver, the same encode/write
// helper the standalone PCF857x expander device uses - but held directly, without that device's own
// state machine: the display's own tick100ms cadence and Starting/Ready states already gate
// readiness. PCF8574-only (8 channels); see Hd44780DisplayDeviceConfigBase for why PCF8575 isn't
// supported here.
class Hd44780I2cCharacterDisplayDeviceBase : public Hd44780CharacterDisplayDeviceBase {
protected:
    using Hd44780CharacterDisplayDeviceBase::Hd44780CharacterDisplayDeviceBase;

    // Supplied by the leaf's config (see Hd44780I2cLeafDeviceBase).
    virtual Hd44780I2cLineChannels i2cLineChannels() const = 0;

    bool setLine(uint8_t lineIndex, bool level, uint32_t now) final;

private:
    Pcf857xIoDriver driver_{8U};
};

} // namespace ewfm
