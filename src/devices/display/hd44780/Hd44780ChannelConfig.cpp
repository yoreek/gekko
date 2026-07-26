#include "devices/display/hd44780/Hd44780ChannelConfig.h"

#include <cstddef>

namespace ewfm {

namespace {
bool channelIsValid(uint8_t channel) {
    return channel <= kHd44780MaxChannel;
}

bool channelIsValidOrUnset(uint8_t channel) {
    return channel == kHd44780ChannelUnset || channelIsValid(channel);
}
} // namespace

bool Hd44780ChannelConfigV1::parseJson(const JsonObjectConst& input, const char*& error) {
    rsChannel = static_cast<uint8_t>(input["rsChannel"] | static_cast<int>(rsChannel));
    eChannel = static_cast<uint8_t>(input["eChannel"] | static_cast<int>(eChannel));
    d4Channel = static_cast<uint8_t>(input["d4Channel"] | static_cast<int>(d4Channel));
    d5Channel = static_cast<uint8_t>(input["d5Channel"] | static_cast<int>(d5Channel));
    d6Channel = static_cast<uint8_t>(input["d6Channel"] | static_cast<int>(d6Channel));
    d7Channel = static_cast<uint8_t>(input["d7Channel"] | static_cast<int>(d7Channel));
    backlightChannel = static_cast<uint8_t>(input["backlightChannel"] | static_cast<int>(backlightChannel));
    (void)error;
    return true;
}

DeviceValidationResult Hd44780ChannelConfigV1::validate() const {
    if (!channelIsValid(rsChannel) || !channelIsValid(eChannel) || !channelIsValid(d4Channel) || !channelIsValid(d5Channel) ||
        !channelIsValid(d6Channel) || !channelIsValid(d7Channel)) {
        return {DeviceError::InvalidConfig, "hd44780 channel is out of range"};
    }
    if (!channelIsValidOrUnset(backlightChannel)) {
        return {DeviceError::InvalidConfig, "hd44780 backlight channel is out of range"};
    }

    const uint8_t channels[7] = {rsChannel, eChannel, d4Channel, d5Channel, d6Channel, d7Channel, backlightChannel};
    for (size_t i = 0; i < 7U; ++i) {
        if (channels[i] == kHd44780ChannelUnset) {
            continue;
        }
        for (size_t j = i + 1U; j < 7U; ++j) {
            if (channels[j] != kHd44780ChannelUnset && channels[i] == channels[j]) {
                return {DeviceError::InvalidConfig, "hd44780 channels must be distinct"};
            }
        }
    }
    return {};
}

void Hd44780ChannelConfigV1::writeJson(JsonObject output) const {
    output["rsChannel"] = rsChannel;
    output["eChannel"] = eChannel;
    output["d4Channel"] = d4Channel;
    output["d5Channel"] = d5Channel;
    output["d6Channel"] = d6Channel;
    output["d7Channel"] = d7Channel;
    output["backlightChannel"] = backlightChannel;
}

uint8_t hd44780ConfigChannels(const Hd44780ChannelConfigV1& config, uint8_t* out, uint8_t maxOut) {
    if (out == nullptr) {
        return 0U;
    }
    const uint8_t channels[7] = {config.rsChannel, config.eChannel,  config.d4Channel,       config.d5Channel,
                                 config.d6Channel, config.d7Channel, config.backlightChannel};
    uint8_t count = 0U;
    for (const uint8_t channel : channels) {
        if (channel == kHd44780ChannelUnset) {
            continue;
        }
        if (count >= maxOut) {
            break;
        }
        out[count++] = channel;
    }
    return count;
}

} // namespace ewfm
