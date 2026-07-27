#include "devices/rtc/common/RtcReadingState.h"

namespace ewfm {

bool RtcReadingState::latestTimeReading(uint32_t& outUtcEpoch, bool& outLostPower) const {
    if (!hasReading_) {
        return false;
    }
    outUtcEpoch = lastEpoch_;
    outLostPower = lastLostPower_;
    return true;
}

bool RtcReadingState::lastReadOk() const {
    return lastReadOk_;
}

uint8_t RtcReadingState::consecutiveErrors() const {
    return consecutiveErrors_;
}

bool RtcReadingState::recordSuccess(uint32_t epoch, bool lostPower) {
    const bool changed = !hasReading_ || !lastReadOk_ || lastLostPower_ != lostPower;
    lastEpoch_ = epoch;
    lastLostPower_ = lostPower;
    hasReading_ = true;
    lastReadOk_ = true;
    consecutiveErrors_ = 0;
    return changed;
}

bool RtcReadingState::recordFailure() {
    const bool changed = lastReadOk_;
    lastReadOk_ = false;
    if (consecutiveErrors_ < 255U) {
        ++consecutiveErrors_;
    }
    return changed;
}

void RtcReadingState::resetErrors() {
    consecutiveErrors_ = 0;
}

} // namespace ewfm
