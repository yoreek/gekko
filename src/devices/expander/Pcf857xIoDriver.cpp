#include "devices/expander/Pcf857xIoDriver.h"

namespace ewfm {

Pcf857xIoDriver::Pcf857xIoDriver(const uint8_t channelCount) : channelCount_(channelCount) {}

uint8_t Pcf857xIoDriver::channelCount() const {
    return channelCount_;
}

uint32_t Pcf857xIoDriver::channelMask() const {
    return (static_cast<uint32_t>(1) << channelCount_) - 1U;
}

bool Pcf857xIoDriver::setChannel(const uint8_t channel, const bool on) {
    if (channel >= channelCount_) {
        return false;
    }
    if (on) {
        channelStates_ |= (static_cast<uint32_t>(1) << channel);
    } else {
        channelStates_ &= ~(static_cast<uint32_t>(1) << channel);
    }
    return true;
}

bool Pcf857xIoDriver::isChannelOn(const uint8_t channel) const {
    if (channel >= channelCount_) {
        return false;
    }
    return (channelStates_ & (static_cast<uint32_t>(1) << channel)) != 0U;
}

uint32_t Pcf857xIoDriver::channelStates() const {
    return channelStates_;
}

bool Pcf857xIoDriver::writeState(II2cBusDriver& driver, const uint8_t i2cAddress, const uint32_t states) const {
    driver.beginTransmission(i2cAddress);
    bool ok;
    if (channelCount_ > 8U) {
        const uint8_t buffer[2] = {
            static_cast<uint8_t>(states & 0xFFU),
            static_cast<uint8_t>((states >> 8) & 0xFFU),
        };
        ok = driver.write(buffer, sizeof(buffer)) == sizeof(buffer);
    } else {
        ok = driver.write(static_cast<uint8_t>(states & 0xFFU)) == 1U;
    }
    return ok && driver.endTransmission(true) == 0U;
}

} // namespace ewfm
