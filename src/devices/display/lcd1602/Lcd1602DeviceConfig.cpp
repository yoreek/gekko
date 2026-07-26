#include "devices/display/lcd1602/Lcd1602DeviceConfig.h"

#include "devices/core/ConfigCodec.h"

#include <cstring>
#include <type_traits>

namespace ewfm {

namespace {
bool channelIsValid(uint8_t channel) {
    return channel <= kLcd1602MaxChannel;
}

bool channelIsValidOrUnset(uint8_t channel) {
    return channel == kLcd1602ChannelUnset || channelIsValid(channel);
}
} // namespace

static_assert(std::is_trivially_copyable<Lcd1602DeviceConfigV1>::value, "Lcd1602DeviceConfigV1 must be POD");
static_assert(sizeof(Lcd1602DeviceConfigV1::kMagic) - 1U + sizeof(Lcd1602DeviceConfigV1) <= kMaxDeviceConfigBytes,
              "Lcd1602DeviceConfigV1 exceeds device config bound");

bool decodeLcd1602DeviceConfig(const uint8_t* blob, size_t size, Lcd1602DeviceConfigV1& config) {
    return decodeValidatedFixedConfigBlob(Lcd1602DeviceConfigV1::kMagic, blob, size, config);
}

bool Lcd1602DeviceConfigV1::parseJson(const JsonObjectConst& input, const char*& error) {
    if (!DeviceBaseConfigV1::parseJson(input, error)) {
        return false;
    }

    rsChannel = static_cast<uint8_t>(input["rsChannel"] | static_cast<int>(rsChannel));
    eChannel = static_cast<uint8_t>(input["eChannel"] | static_cast<int>(eChannel));
    d4Channel = static_cast<uint8_t>(input["d4Channel"] | static_cast<int>(d4Channel));
    d5Channel = static_cast<uint8_t>(input["d5Channel"] | static_cast<int>(d5Channel));
    d6Channel = static_cast<uint8_t>(input["d6Channel"] | static_cast<int>(d6Channel));
    d7Channel = static_cast<uint8_t>(input["d7Channel"] | static_cast<int>(d7Channel));
    backlightChannel = static_cast<uint8_t>(input["backlightChannel"] | static_cast<int>(backlightChannel));

    char previousLine1[kLcd1602LineLength + 1U]{};
    std::memcpy(previousLine1, line1, sizeof(previousLine1));
    const char* newLine1 = input["line1"] | static_cast<const char*>(previousLine1);
    if (!copyBoundedText(line1, newLine1)) {
        error = "lcd1602 line1 exceeds 16 characters";
        return false;
    }

    char previousLine2[kLcd1602LineLength + 1U]{};
    std::memcpy(previousLine2, line2, sizeof(previousLine2));
    const char* newLine2 = input["line2"] | static_cast<const char*>(previousLine2);
    if (!copyBoundedText(line2, newLine2)) {
        error = "lcd1602 line2 exceeds 16 characters";
        return false;
    }

    return true;
}

DeviceValidationResult Lcd1602DeviceConfigV1::validate() const {
    const DeviceValidationResult baseResult = DeviceBaseConfigV1::validate();
    if (!baseResult.ok()) {
        return baseResult;
    }
    if (!channelIsValid(rsChannel) || !channelIsValid(eChannel) || !channelIsValid(d4Channel) || !channelIsValid(d5Channel) ||
        !channelIsValid(d6Channel) || !channelIsValid(d7Channel)) {
        return {DeviceError::InvalidConfig, "lcd1602 channel is out of range"};
    }
    if (!channelIsValidOrUnset(backlightChannel)) {
        return {DeviceError::InvalidConfig, "lcd1602 backlight channel is out of range"};
    }

    const uint8_t channels[7] = {rsChannel, eChannel, d4Channel, d5Channel, d6Channel, d7Channel, backlightChannel};
    for (size_t i = 0; i < 7U; ++i) {
        if (channels[i] == kLcd1602ChannelUnset) {
            continue;
        }
        for (size_t j = i + 1U; j < 7U; ++j) {
            if (channels[j] != kLcd1602ChannelUnset && channels[i] == channels[j]) {
                return {DeviceError::InvalidConfig, "lcd1602 channels must be distinct"};
            }
        }
    }
    return {};
}

uint8_t lcd1602ConfigChannels(const Lcd1602DeviceConfigV1& config, uint8_t* out, uint8_t maxOut) {
    if (out == nullptr) {
        return 0U;
    }
    const uint8_t channels[7] = {config.rsChannel, config.eChannel,  config.d4Channel,       config.d5Channel,
                                 config.d6Channel, config.d7Channel, config.backlightChannel};
    uint8_t count = 0U;
    for (const uint8_t channel : channels) {
        if (channel == kLcd1602ChannelUnset) {
            continue;
        }
        if (count >= maxOut) {
            break;
        }
        out[count++] = channel;
    }
    return count;
}

void Lcd1602DeviceConfigV1::writeJson(JsonObject output) const {
    DeviceBaseConfigV1::writeJson(output);
    output["rsChannel"] = rsChannel;
    output["eChannel"] = eChannel;
    output["d4Channel"] = d4Channel;
    output["d5Channel"] = d5Channel;
    output["d6Channel"] = d6Channel;
    output["d7Channel"] = d7Channel;
    output["backlightChannel"] = backlightChannel;
    output["line1"] = JsonString(line1, JsonString::Copied);
    output["line2"] = JsonString(line2, JsonString::Copied);
}

} // namespace ewfm
