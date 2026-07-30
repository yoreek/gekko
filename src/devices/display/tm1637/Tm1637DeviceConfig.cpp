#include "devices/display/tm1637/Tm1637DeviceConfig.h"

#include "devices/core/ConfigCodec.h"
#include "devices/switch/gpio/GpioSwitchDeviceConfig.h"

#include <cstring>
#include <type_traits>

namespace ewfm {

namespace {

bool parsePinField(const JsonObjectConst& input, const char* key, uint8_t& pin, const char*& error) {
    const JsonVariantConst value = input[key];
    if (value.isNull()) {
        return true;
    }
    if (!value.is<unsigned long>() && !value.is<long>() && !value.is<int>()) {
        error = "device pin must be numeric";
        return false;
    }
    const long parsed = value.as<long>();
    if (parsed < 0L || parsed > 255L) {
        error = "device pin is out of bounds";
        return false;
    }
    pin = static_cast<uint8_t>(parsed);
    return true;
}

} // namespace

EWFM_LEGACY_CONFIG_USE_BEGIN
static_assert(std::is_trivially_copyable<Tm1637DeviceConfigV1>::value, "Tm1637DeviceConfigV1 must be POD");
static_assert(sizeof(Tm1637DeviceConfigV1) == 37, "Tm1637DeviceConfigV1 layout changed");
EWFM_LEGACY_CONFIG_USE_END

static_assert(std::is_trivially_copyable<Tm1637DeviceConfigV2>::value, "Tm1637DeviceConfigV2 must be POD");
static_assert(sizeof(Tm1637DeviceConfigV2) == 39, "Tm1637DeviceConfigV2 layout changed");
static_assert(sizeof(Tm1637DeviceConfigV2::kMagic) - 1U + sizeof(Tm1637DeviceConfigV2) <= kMaxDeviceConfigBytes,
              "Tm1637DeviceConfigV2 exceeds device config bound");

bool decodeTm1637DeviceConfig(const uint8_t* blob, const size_t size, Tm1637DeviceConfigV2& config) {
    if (decodeValidatedFixedConfigBlob(Tm1637DeviceConfigV2::kMagic, blob, size, config)) {
        return true;
    }

    EWFM_LEGACY_CONFIG_USE_BEGIN
    Tm1637DeviceConfigV1 legacy{};
    if (!decodeFixedConfigBlob(Tm1637DeviceConfigV1::kMagic, blob, size, legacy) || !legacy.validate().ok()) {
        return false;
    }
    config.migrateFrom(legacy);
    EWFM_LEGACY_CONFIG_USE_END
    return config.validate().ok();
}

EWFM_LEGACY_CONFIG_USE_BEGIN
DeviceValidationResult Tm1637DeviceConfigV1::validate() const {
    const DeviceValidationResult baseResult = DeviceBaseConfigV1::validate();
    if (!baseResult.ok()) {
        return baseResult;
    }

    switch (static_cast<Tm1637PanelKind>(panel)) {
    case Tm1637PanelKind::FourDigitDecimal036:
        break;
    default:
        return {DeviceError::InvalidConfig, "display panel is invalid"};
    }
    if (brightness > 7U) {
        return {DeviceError::InvalidConfig, "display brightness is out of bounds"};
    }
    if (rotation != 0U && rotation != 180U) {
        return {DeviceError::InvalidConfig, "display rotation is out of bounds"};
    }
    return {};
}

void Tm1637DeviceConfigV2::migrateFrom(const Tm1637DeviceConfigV1& legacy) {
    static_cast<DeviceBaseConfigV1&>(*this) = static_cast<const DeviceBaseConfigV1&>(legacy);
    panel = legacy.panel;
    brightness = legacy.brightness;
    rotation = legacy.rotation;
    // V1 kept CLK/DIO as switch dependencies, which this struct cannot see: leave both unset so the
    // device faults visibly instead of driving whichever pins happened to be the defaults.
    clkPin = kTm1637UnsetPin;
    dioPin = kTm1637UnsetPin;
}
EWFM_LEGACY_CONFIG_USE_END

bool Tm1637DeviceConfigV2::pinsConfigured() const {
    return clkPin != kTm1637UnsetPin && dioPin != kTm1637UnsetPin;
}

bool Tm1637DeviceConfigV2::parseJson(const JsonObjectConst& input, const char*& error) {
    if (!parseDeviceBaseConfigJson(input, *this, error)) {
        return false;
    }

    const JsonVariantConst panelValue = input["panel"];
    Tm1637PanelKind panelKind{};
    if (panelValue.isNull()) {
        panelKind = Tm1637PanelKind::FourDigitDecimal036;
    } else if (!tm1637PanelKindFromString(panelValue.as<const char*>(), panelKind)) {
        error = "display panel is invalid";
        return false;
    }
    panel = static_cast<uint8_t>(panelKind);

    const JsonVariantConst brightnessValue = input["brightness"];
    if (!brightnessValue.isNull()) {
        if (!brightnessValue.is<unsigned long>() && !brightnessValue.is<long>() && !brightnessValue.is<int>()) {
            error = "display brightness must be numeric";
            return false;
        }
        const long parsed = brightnessValue.as<long>();
        if (parsed < 0L || parsed > 7L) {
            error = "display brightness is out of bounds";
            return false;
        }
        brightness = static_cast<uint8_t>(parsed);
    }

    const JsonVariantConst rotationValue = input["rotation"];
    if (!rotationValue.isNull()) {
        if (!rotationValue.is<unsigned long>() && !rotationValue.is<long>() && !rotationValue.is<int>()) {
            error = "display rotation must be numeric";
            return false;
        }
        const long parsed = rotationValue.as<long>();
        if (parsed != 0L && parsed != 180L) {
            error = "display rotation is out of bounds";
            return false;
        }
        rotation = static_cast<uint8_t>(parsed);
    }

    if (!parsePinField(input, "clkPin", clkPin, error) || !parsePinField(input, "dioPin", dioPin, error)) {
        return false;
    }
    // validate() tolerates the unset pair so migrated V1 blobs stay decodable; REST does not.
    if (!pinsConfigured()) {
        error = "device pins are required";
        return false;
    }

    const DeviceValidationResult result = validate();
    if (!result.ok()) {
        error = result.message;
        return false;
    }
    return true;
}

DeviceValidationResult Tm1637DeviceConfigV2::validate() const {
    const DeviceValidationResult baseResult = DeviceBaseConfigV1::validate();
    if (!baseResult.ok()) {
        return baseResult;
    }

    switch (static_cast<Tm1637PanelKind>(panel)) {
    case Tm1637PanelKind::FourDigitDecimal036:
        break;
    default:
        return {DeviceError::InvalidConfig, "display panel is invalid"};
    }
    if (brightness > 7U) {
        return {DeviceError::InvalidConfig, "display brightness is out of bounds"};
    }
    if (rotation != 0U && rotation != 180U) {
        return {DeviceError::InvalidConfig, "display rotation is out of bounds"};
    }
    if (clkPin == kTm1637UnsetPin && dioPin == kTm1637UnsetPin) {
        // Migrated from V1: valid as stored, unusable until the portal supplies the pins.
        return {};
    }
    if (!gpioSwitchPinIsValid(clkPin)) {
        return {DeviceError::InvalidConfig, "device clk pin is invalid"};
    }
    if (!gpioSwitchPinIsValid(dioPin)) {
        return {DeviceError::InvalidConfig, "device dio pin is invalid"};
    }
    if (clkPin == dioPin) {
        return {DeviceError::InvalidConfig, "device pins must be distinct"};
    }
    return {};
}

void Tm1637DeviceConfigV2::writeJson(JsonObject output) const {
    writeDeviceBaseConfigJson(*this, output);
    output["panel"] = tm1637PanelKindName(static_cast<Tm1637PanelKind>(panel));
    output["brightness"] = brightness;
    output["rotation"] = rotation;
    output["clkPin"] = clkPin;
    output["dioPin"] = dioPin;
}

} // namespace ewfm
