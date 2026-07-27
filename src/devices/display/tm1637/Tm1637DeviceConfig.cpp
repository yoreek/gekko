#include "devices/display/tm1637/Tm1637DeviceConfig.h"

#include "devices/core/ConfigCodec.h"

#include <cstring>
#include <type_traits>

namespace ewfm {

static_assert(std::is_trivially_copyable<Tm1637DeviceConfigV1>::value, "Tm1637DeviceConfigV1 must be POD");
static_assert(sizeof(Tm1637DeviceConfigV1) == 37, "Tm1637DeviceConfigV1 layout changed");
static_assert(sizeof(Tm1637DeviceConfigV1::kMagic) - 1U + sizeof(Tm1637DeviceConfigV1) <= kMaxDeviceConfigBytes,
              "Tm1637DeviceConfigV1 exceeds device config bound");

bool decodeTm1637DeviceConfig(const uint8_t* blob, const size_t size, Tm1637DeviceConfigV1& config) {
    return decodeValidatedFixedConfigBlob(Tm1637DeviceConfigV1::kMagic, blob, size, config);
}

bool Tm1637DeviceConfigV1::parseJson(const JsonObjectConst& input, const char*& error) {
    if (!parseDeviceBaseConfigJson(input, *this, error)) {
        return false;
    }

    const JsonVariantConst panelValue = input["panel"];
    Tm1637PanelKind panelKind{};
    if (panelValue.isNull()) {
        panelKind = Tm1637PanelKind::FourDigitDecimal036;
    } else if (!tm1637PanelKindFromString(panelValue.as<const char*>(), panelKind)) {
        error = "tm1637 panel is invalid";
        return false;
    }
    panel = static_cast<uint8_t>(panelKind);

    const JsonVariantConst brightnessValue = input["brightness"];
    if (!brightnessValue.isNull()) {
        if (!brightnessValue.is<unsigned long>() && !brightnessValue.is<long>() && !brightnessValue.is<int>()) {
            error = "tm1637 brightness must be numeric";
            return false;
        }
        const long parsed = brightnessValue.as<long>();
        if (parsed < 0L || parsed > 7L) {
            error = "tm1637 brightness is out of bounds";
            return false;
        }
        brightness = static_cast<uint8_t>(parsed);
    }

    const JsonVariantConst rotationValue = input["rotation"];
    if (!rotationValue.isNull()) {
        if (!rotationValue.is<unsigned long>() && !rotationValue.is<long>() && !rotationValue.is<int>()) {
            error = "tm1637 rotation must be numeric";
            return false;
        }
        const long parsed = rotationValue.as<long>();
        if (parsed != 0L && parsed != 180L) {
            error = "tm1637 rotation is out of bounds";
            return false;
        }
        rotation = static_cast<uint8_t>(parsed);
    }

    return true;
}

DeviceValidationResult Tm1637DeviceConfigV1::validate() const {
    const DeviceValidationResult baseResult = DeviceBaseConfigV1::validate();
    if (!baseResult.ok()) {
        return baseResult;
    }

    switch (static_cast<Tm1637PanelKind>(panel)) {
    case Tm1637PanelKind::FourDigitDecimal036:
        break;
    default:
        return {DeviceError::InvalidConfig, "tm1637 panel is invalid"};
    }
    if (brightness > 7U) {
        return {DeviceError::InvalidConfig, "tm1637 brightness is out of bounds"};
    }
    if (rotation != 0U && rotation != 180U) {
        return {DeviceError::InvalidConfig, "tm1637 rotation is out of bounds"};
    }
    return {};
}

void Tm1637DeviceConfigV1::writeJson(JsonObject output) const {
    writeDeviceBaseConfigJson(*this, output);
    output["panel"] = tm1637PanelKindName(static_cast<Tm1637PanelKind>(panel));
    output["brightness"] = brightness;
    output["rotation"] = rotation;
}

} // namespace ewfm
