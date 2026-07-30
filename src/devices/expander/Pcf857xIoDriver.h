#pragma once

#include "devices/bus/i2c/II2cBusDriver.h"

#include <cstdint>

namespace ewfm {

// Pure encode/write helper for the PCF857x family of I2C GPIO expanders (PCF8574 = 8 channels,
// single-byte register; PCF8575 = 16 channels, two-byte LE register). Holds only an in-memory
// output bitmask - no state machine, no retry/backoff, no bus-dependency lookup. Shared by
// Pcf857xExpanderDeviceBase (the standalone, retried, periodically-resynced expander device) and
// Hd44780I2cCharacterDisplayDeviceBase (an embedded PCF8574 backpack driven at the display's own
// tick cadence, without a separate state machine).
class Pcf857xIoDriver {
public:
    explicit Pcf857xIoDriver(uint8_t channelCount);

    uint8_t channelCount() const;
    uint32_t channelMask() const;

    // Returns false (state unchanged) if channel >= channelCount().
    bool setChannel(uint8_t channel, bool on);
    bool isChannelOn(uint8_t channel) const;
    uint32_t channelStates() const;

    // Writes `states` (caller applies any inversion first) to the chip at `i2cAddress`:
    // beginTransmission, a 1- or 2-byte write depending on channelCount(), endTransmission.
    bool writeState(II2cBusDriver& driver, uint8_t i2cAddress, uint32_t states) const;

private:
    uint8_t channelCount_;
    uint32_t channelStates_{0U};
};

} // namespace ewfm
