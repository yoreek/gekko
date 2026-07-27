#include "devices/rtc/ds1302/Ds1302RtcDeviceConfig.h"

#include "devices/core/ConfigCodec.h"
#include "devices/rtc/common/RtcSyncConfig.h"
#include "devices/switch/gpio/GpioSwitchDeviceConfig.h"

#include <type_traits>

namespace ewfm {

namespace {

bool parseUint32(const JsonVariantConst& variant, uint32_t& value) {
    if (variant.isNull()) {
        return true;
    }
    if (!variant.is<unsigned long>() && !variant.is<unsigned int>() && !variant.is<int>()) {
        return false;
    }
    const long parsed = variant.as<long>();
    if (parsed < 0) {
        return false;
    }
    value = static_cast<uint32_t>(parsed);
    return true;
}

bool parsePinField(const JsonObjectConst& input, const char* key, uint8_t& pin, const char*& error) {
    uint32_t value = pin;
    if (!parseUint32(input[key], value) || value > 255U) {
        error = "ds1302 pin must be numeric";
        return false;
    }
    pin = static_cast<uint8_t>(value);
    return true;
}

bool parseDs1302FieldsJson(const JsonObjectConst& input, Ds1302RtcDeviceConfigV1& config, const char*& error) {
    if (!parsePinField(input, "clkPin", config.clkPin, error) || !parsePinField(input, "dataPin", config.dataPin, error) ||
        !parsePinField(input, "rstPin", config.rstPin, error)) {
        return false;
    }
    return parseRtcSyncFieldJson(input, config.useForSystemTimeSync, error);
}

void writeDs1302FieldsJson(const Ds1302RtcDeviceConfigV1& config, JsonObject output) {
    output["clkPin"] = config.clkPin;
    output["dataPin"] = config.dataPin;
    output["rstPin"] = config.rstPin;
    writeRtcSyncFieldJson(config.useForSystemTimeSync, output);
}

} // namespace

static_assert(std::is_trivially_copyable<Ds1302RtcDeviceConfigV1>::value, "Ds1302RtcDeviceConfigV1 must be POD");
static_assert(sizeof(Ds1302RtcDeviceConfigV1::kMagic) - 1U + sizeof(Ds1302RtcDeviceConfigV1) <= kMaxDeviceConfigBytes,
              "Ds1302RtcDeviceConfigV1 exceeds device config bound");

bool decodeDs1302RtcDeviceConfig(const uint8_t* blob, size_t size, Ds1302RtcDeviceConfigV1& config) {
    return decodeFixedConfigBlob(Ds1302RtcDeviceConfigV1::kMagic, blob, size, config) && config.validate().ok();
}

bool parseDs1302RtcDeviceConfigJson(const JsonObjectConst& input, Ds1302RtcDeviceConfigV1& config, const char*& error) {
    return config.parseJson(input, error);
}

void writeDs1302RtcDeviceConfigJson(const Ds1302RtcDeviceConfigV1& config, JsonObject output) {
    config.writeJson(output);
}

DeviceValidationResult Ds1302RtcDeviceConfigV1::validate() const {
    const DeviceValidationResult baseValidation = DeviceBaseConfigV1::validate();
    if (!baseValidation.ok()) {
        return baseValidation;
    }
    const DeviceValidationResult syncValidation = validateRtcSyncField(useForSystemTimeSync);
    if (!syncValidation.ok()) {
        return syncValidation;
    }
    if (!gpioSwitchPinIsValid(clkPin)) {
        return {DeviceError::InvalidConfig, "ds1302 clk pin is invalid"};
    }
    if (!gpioSwitchPinIsValid(dataPin)) {
        return {DeviceError::InvalidConfig, "ds1302 data pin is invalid"};
    }
    if (!gpioSwitchPinIsValid(rstPin)) {
        return {DeviceError::InvalidConfig, "ds1302 rst pin is invalid"};
    }
    if (clkPin == dataPin || clkPin == rstPin || dataPin == rstPin) {
        return {DeviceError::InvalidConfig, "ds1302 pins must be distinct"};
    }
    return {};
}

bool Ds1302RtcDeviceConfigV1::parseJson(const JsonObjectConst& input, const char*& error) {
    if (!DeviceBaseConfigV1::parseJson(input, error) || !parseDs1302FieldsJson(input, *this, error)) {
        return false;
    }
    const DeviceValidationResult result = validate();
    if (!result.ok()) {
        error = result.message;
        return false;
    }
    return true;
}

void Ds1302RtcDeviceConfigV1::writeJson(JsonObject output) const {
    DeviceBaseConfigV1::writeJson(output);
    writeDs1302FieldsJson(*this, output);
}

} // namespace ewfm
