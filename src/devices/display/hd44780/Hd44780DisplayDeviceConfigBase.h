#pragma once

#include "devices/core/DeviceBaseConfig.h"
#include "devices/display/hd44780/Hd44780ChannelConfig.h"

namespace ewfm {

#pragma pack(push, 1)
template <typename Derived> struct Hd44780DisplayDeviceConfigBase : DeviceBaseConfigV1 {
    Hd44780ChannelConfigV1 channels{};

    DeviceValidationResult validate() const {
        const DeviceValidationResult baseResult = DeviceBaseConfigV1::validate();
        return baseResult.ok() ? channels.validate() : baseResult;
    }

    bool parseJson(const JsonObjectConst& input, const char*& error) {
        return parseDeviceBaseConfigJson(input, *this, error) && channels.parseJson(input, error);
    }

    void writeJson(JsonObject output) const {
        writeDeviceBaseConfigJson(*this, output);
        channels.writeJson(output);
    }
};
#pragma pack(pop)

template <typename Config> constexpr size_t hd44780DisplayDeviceConfigSize(const Config&) {
    return sizeof(Config::kMagic) - 1U + sizeof(Config);
}

template <typename Config> bool decodeHd44780DisplayDeviceConfig(const uint8_t* blob, size_t size, Config& config) {
    return decodeValidatedFixedConfigBlob(Config::kMagic, blob, size, config);
}

} // namespace ewfm
