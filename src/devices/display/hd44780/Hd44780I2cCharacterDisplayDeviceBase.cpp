#include "devices/display/hd44780/Hd44780I2cCharacterDisplayDeviceBase.h"

namespace ewfm {

bool Hd44780I2cCharacterDisplayDeviceBase::setLine(const uint8_t lineIndex, const bool level, const uint32_t now) {
    (void)now;
    const Hd44780I2cLineChannels channels = i2cLineChannels();
    uint8_t channel;
    switch (lineIndex) {
    case kHd44780LineRs:
        channel = channels.rsChannel;
        break;
    case kHd44780LineE:
        channel = channels.eChannel;
        break;
    case kHd44780LineD4:
        channel = channels.d4Channel;
        break;
    case kHd44780LineD5:
        channel = channels.d5Channel;
        break;
    case kHd44780LineD6:
        channel = channels.d6Channel;
        break;
    case kHd44780LineD7:
        channel = channels.d7Channel;
        break;
    case kHd44780LineBacklight:
        channel = channels.backlightChannel;
        if (channel == kHd44780ChannelUnset) {
            return true;
        }
        break;
    default:
        return false;
    }

    if (!driver_.setChannel(channel, level)) {
        return false;
    }

    I2cBusDevice* bus = static_cast<I2cBusDevice*>(dependencyRuntime(DeviceRole::I2CBus));
    if (bus == nullptr) {
        return false;
    }
    I2cBusDevice::DependencyTransaction transaction = bus->beginDependencyTransaction();
    if (!transaction) {
        return false;
    }
    return driver_.writeState(*transaction.driver(), channels.i2cAddress, driver_.channelStates());
}

} // namespace ewfm
