#pragma once

#include "devices/analog/input/ads1115/Ads1115Protocol.h"
#include "devices/core/DeviceBaseConfig.h"

#include <ArduinoJson.h>
#include <cstddef>
#include <cstdint>

namespace ewfm {

constexpr DeviceTypeId kAds1115HubTypeId = 25;
constexpr uint32_t kAds1115HubConfigVersion = 1;
constexpr uint8_t kAds1115DefaultI2cAddress = 0x48;

#pragma pack(push, 1)
struct Ads1115HubDeviceConfigV1 : DeviceBaseConfigV1 {
    static constexpr char kMagic[] = "ADS1115-HUB-1";

    uint8_t i2cAddress{kAds1115DefaultI2cAddress};
    uint8_t gain{static_cast<uint8_t>(Ads1115Gain::Fsr2048)};
    uint8_t dataRateSps{static_cast<uint8_t>(Ads1115DataRate::Sps128)};

    bool parseJson(const JsonObjectConst& input, const char*& error);
    DeviceValidationResult validate() const;
    void writeJson(JsonObject output) const;
};
#pragma pack(pop)

constexpr size_t ads1115HubDeviceConfigSize(const Ads1115HubDeviceConfigV1&) {
    return sizeof(Ads1115HubDeviceConfigV1::kMagic) - 1U + sizeof(Ads1115HubDeviceConfigV1);
}

bool decodeAds1115HubDeviceConfig(const uint8_t* blob, size_t size, Ads1115HubDeviceConfigV1& config);

} // namespace ewfm
