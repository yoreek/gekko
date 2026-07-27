#pragma once

#include <cstdint>

namespace ewfm {

// Bookkeeping shared by every IRealTimeClockRuntime-implementing device (DS3231, DS1302, ...):
// the last successfully read epoch/lost-power flag, whether a reading has ever been taken, and the
// consecutive-failure counter that drives retry/fault backoff. Transport-specific concerns (I2C vs
// bit-bang GPIO, register layout, retry deadlines) stay on the concrete device; this only owns the
// "what did we last read, and is it still fresh" bookkeeping that every RTC device duplicates.
class RtcReadingState {
public:
    bool latestTimeReading(uint32_t& outUtcEpoch, bool& outLostPower) const;
    bool lastReadOk() const;
    uint8_t consecutiveErrors() const;

    // Returns true if the caller should mark runtime state dirty (a real change in read status, not
    // a bare epoch tick - callers reading the current epoch on demand don't need a push per reading).
    bool recordSuccess(uint32_t epoch, bool lostPower);
    // Returns true if the caller should mark runtime state dirty (read status flipped to failed).
    bool recordFailure();
    void resetErrors();

private:
    uint32_t lastEpoch_{0};
    bool lastLostPower_{false};
    bool hasReading_{false};
    bool lastReadOk_{false};
    uint8_t consecutiveErrors_{0};
};

} // namespace ewfm
