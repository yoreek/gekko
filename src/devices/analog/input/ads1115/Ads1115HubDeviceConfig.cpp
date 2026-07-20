#include "devices/analog/input/ads1115/Ads1115HubDeviceConfig.h"

#include "devices/core/ConfigCodec.h"

#include <type_traits>

namespace ewfm {

namespace {
bool parseUint8(const JsonVariantConst& variant, uint8_t& value) {
    if (variant.isNull()) {
        return true;
    }
    if (!variant.is<unsigned int>() && !variant.is<int>()) {
        return false;
    }
    const long parsed = variant.as<long>();
    if (parsed < 0 || parsed > 255L) {
        return false;
    }
    value = static_cast<uint8_t>(parsed);
    return true;
}
} // namespace

static_assert(std::is_trivially_copyable<Ads1115HubDeviceConfigV1>::value, "Ads1115HubDeviceConfigV1 must be POD");
static_assert(sizeof(Ads1115HubDeviceConfigV1::kMagic) - 1U + sizeof(Ads1115HubDeviceConfigV1) <= kMaxDeviceConfigBytes,
              "Ads1115HubDeviceConfigV1 exceeds device config bound");

bool decodeAds1115HubDeviceConfig(const uint8_t* blob, size_t size, Ads1115HubDeviceConfigV1& config) {
    return decodeValidatedFixedConfigBlob(Ads1115HubDeviceConfigV1::kMagic, blob, size, config);
}

DeviceValidationResult Ads1115HubDeviceConfigV1::validate() const {
    const DeviceValidationResult baseValidation = DeviceBaseConfigV1::validate();
    if (!baseValidation.ok()) {
        return baseValidation;
    }
    if (i2cAddress < 0x08U || i2cAddress > 0x77U) {
        return {DeviceError::InvalidConfig, "ads1115 i2c address is invalid"};
    }
    Ads1115Gain parsedGain{};
    if (!ads1115GainFromByte(gain, parsedGain)) {
        return {DeviceError::InvalidConfig, "ads1115 gain is invalid"};
    }
    Ads1115DataRate parsedRate{};
    if (!ads1115DataRateFromByte(dataRateSps, parsedRate)) {
        return {DeviceError::InvalidConfig, "ads1115 data rate is invalid"};
    }
    return {};
}

bool Ads1115HubDeviceConfigV1::parseJson(const JsonObjectConst& input, const char*& error) {
    if (!DeviceBaseConfigV1::parseJson(input, error)) {
        return false;
    }

    uint8_t address = i2cAddress;
    if (!parseUint8(input["i2cAddress"], address)) {
        error = "ads1115 i2c address must be numeric";
        return false;
    }
    i2cAddress = address;

    Ads1115Gain parsedGain{};
    if (!ads1115GainFromByte(gain, parsedGain)) {
        parsedGain = Ads1115Gain::Fsr2048;
    }
    if (!ads1115GainFromString(input["gain"] | ads1115GainName(parsedGain), parsedGain)) {
        error = "ads1115 gain is invalid";
        return false;
    }
    gain = static_cast<uint8_t>(parsedGain);

    Ads1115DataRate parsedRate{};
    if (!ads1115DataRateFromByte(dataRateSps, parsedRate)) {
        parsedRate = Ads1115DataRate::Sps128;
    }
    if (!ads1115DataRateFromString(input["dataRateSps"] | ads1115DataRateName(parsedRate), parsedRate)) {
        error = "ads1115 data rate is invalid";
        return false;
    }
    dataRateSps = static_cast<uint8_t>(parsedRate);

    const DeviceValidationResult result = validate();
    if (!result.ok()) {
        error = result.message;
        return false;
    }
    return true;
}

void Ads1115HubDeviceConfigV1::writeJson(JsonObject output) const {
    DeviceBaseConfigV1::writeJson(output);
    output["i2cAddress"] = i2cAddress;
    Ads1115Gain parsedGain{};
    (void)ads1115GainFromByte(gain, parsedGain);
    output["gain"] = ads1115GainName(parsedGain);
    Ads1115DataRate parsedRate{};
    (void)ads1115DataRateFromByte(dataRateSps, parsedRate);
    output["dataRateSps"] = ads1115DataRateName(parsedRate);
}

} // namespace ewfm
