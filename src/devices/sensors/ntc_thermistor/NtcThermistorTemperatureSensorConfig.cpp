#include "devices/sensors/ntc_thermistor/NtcThermistorTemperatureSensorConfig.h"

#include "devices/core/ConfigCodec.h"

#include <cstring>
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

bool parseUint16(const JsonVariantConst& variant, uint16_t& value) {
    uint32_t parsed = value;
    if (!parseUint32(variant, parsed) || parsed > 65535UL) {
        return false;
    }
    value = static_cast<uint16_t>(parsed);
    return true;
}

bool parseFloat(const JsonVariantConst& variant, float& value) {
    if (variant.isNull()) {
        return true;
    }
    if (!variant.is<float>() && !variant.is<double>() && !variant.is<int>()) {
        return false;
    }
    value = variant.as<float>();
    return true;
}

bool parseBoolField(const JsonObjectConst& input, const char* key, bool defaultValue) {
    return (input[key] | defaultValue) ? true : false;
}

bool parseReportDelta(const JsonObjectConst& input, uint16_t& centiCelsius) {
    const JsonVariantConst centiVariant = input["reportDeltaCentiCelsius"];
    if (!centiVariant.isNull()) {
        return parseUint16(centiVariant, centiCelsius);
    }

    const JsonVariantConst celsiusVariant = input["reportDeltaCelsius"];
    if (celsiusVariant.isNull()) {
        return true;
    }
    if (!celsiusVariant.is<float>() && !celsiusVariant.is<double>() && !celsiusVariant.is<int>()) {
        return false;
    }
    const float celsius = celsiusVariant.as<float>();
    if (celsius < 0.01F || celsius > 655.35F) {
        return false;
    }
    centiCelsius = static_cast<uint16_t>(celsius * 100.0F + 0.5F);
    return centiCelsius != 0U;
}

bool parseNominalTemp(const JsonObjectConst& input, int16_t& centiCelsius) {
    const JsonVariantConst centiVariant = input["nominalTempCentiCelsius"];
    if (!centiVariant.isNull()) {
        if (!centiVariant.is<int>() && !centiVariant.is<long>()) {
            return false;
        }
        const long parsed = centiVariant.as<long>();
        if (parsed < -32768L || parsed > 32767L) {
            return false;
        }
        centiCelsius = static_cast<int16_t>(parsed);
        return true;
    }

    const JsonVariantConst celsiusVariant = input["nominalTempCelsius"];
    if (celsiusVariant.isNull()) {
        return true;
    }
    if (!celsiusVariant.is<float>() && !celsiusVariant.is<double>() && !celsiusVariant.is<int>()) {
        return false;
    }
    const float celsius = celsiusVariant.as<float>();
    if (celsius < -327.68F || celsius > 327.67F) {
        return false;
    }
    centiCelsius = static_cast<int16_t>(celsius * 100.0F + (celsius >= 0.0F ? 0.5F : -0.5F));
    return true;
}

} // namespace

static_assert(std::is_trivially_copyable<NtcThermistorTemperatureSensorConfigV1>::value,
              "NtcThermistorTemperatureSensorConfigV1 must be POD");
static_assert(sizeof(NtcThermistorTemperatureSensorConfigV1::kMagic) - 1U + sizeof(NtcThermistorTemperatureSensorConfigV1) <=
                  kMaxDeviceConfigBytes,
              "NtcThermistorTemperatureSensorConfigV1 exceeds device config bound");

bool parseNtcThermistorTemperatureSensorConfigJson(const JsonObjectConst& input, NtcThermistorTemperatureSensorConfigV1& config,
                                                   const char*& error) {
    return config.parseJson(input, error);
}

void writeNtcThermistorTemperatureSensorConfigJson(const NtcThermistorTemperatureSensorConfigV1& config, JsonObject output) {
    config.writeJson(output);
}

DeviceValidationResult NtcThermistorTemperatureSensorConfigV1::validate() const {
    const DeviceValidationResult baseValidation = DeviceBaseConfigV1::validate();
    if (!baseValidation.ok()) {
        return baseValidation;
    }
    NtcFormulaMode mode{};
    if (!ntcFormulaModeFromByte(formulaMode, mode)) {
        return {DeviceError::InvalidConfig, "ntc thermistor formula mode is invalid"};
    }
    if (seriesResistorOhms == 0U) {
        return {DeviceError::InvalidConfig, "ntc thermistor series resistor is invalid"};
    }
    if (supplyMilliVolts == 0U) {
        return {DeviceError::InvalidConfig, "ntc thermistor supply voltage is invalid"};
    }
    if (mode == NtcFormulaMode::Beta) {
        if (nominalResistanceOhms == 0U) {
            return {DeviceError::InvalidConfig, "ntc thermistor nominal resistance is invalid"};
        }
        if (betaCoefficient == 0U) {
            return {DeviceError::InvalidConfig, "ntc thermistor beta coefficient is invalid"};
        }
    } else {
        if (steinhartA == 0.0F && steinhartB == 0.0F && steinhartC == 0.0F) {
            return {DeviceError::InvalidConfig, "ntc thermistor steinhart-hart coefficients are invalid"};
        }
    }
    TemperatureUnit unit{};
    if (!temperatureUnitFromByte(outputUnit, unit)) {
        return {DeviceError::InvalidConfig, "ntc thermistor output unit is invalid"};
    }
    if (reportAlways > 1U) {
        return {DeviceError::InvalidConfig, "ntc thermistor report policy is invalid"};
    }
    if (reportDeltaCentiCelsius == 0U) {
        return {DeviceError::InvalidConfig, "ntc thermistor report delta is invalid"};
    }
    if (pollMs < kNtcThermistorMinPollMs || pollMs > kNtcThermistorMaxPollMs) {
        return {DeviceError::InvalidConfig, "ntc thermistor poll period is invalid"};
    }
    return filter.validate();
}

bool NtcThermistorTemperatureSensorConfigV1::parseJson(const JsonObjectConst& input, const char*& error) {
    if (!DeviceBaseConfigV1::parseJson(input, error)) {
        return false;
    }

    reportAlways = parseBoolField(input, "reportAlways", reportAlways != 0U) ? 1U : 0U;

    NtcFormulaMode parsedMode{};
    if (!ntcFormulaModeFromByte(formulaMode, parsedMode)) {
        parsedMode = NtcFormulaMode::Beta;
    }
    if (!ntcFormulaModeFromString(input["formulaMode"] | ntcFormulaModeName(parsedMode), parsedMode)) {
        error = "ntc thermistor formula mode is invalid";
        return false;
    }
    formulaMode = static_cast<uint8_t>(parsedMode);

    uint16_t seriesResistor = seriesResistorOhms;
    if (!parseUint16(input["seriesResistorOhms"], seriesResistor)) {
        error = "ntc thermistor series resistor must be numeric";
        return false;
    }
    seriesResistorOhms = seriesResistor;

    uint16_t supply = supplyMilliVolts;
    if (!parseUint16(input["supplyMilliVolts"], supply)) {
        error = "ntc thermistor supply voltage must be numeric";
        return false;
    }
    supplyMilliVolts = supply;

    uint32_t nominalResistance = nominalResistanceOhms;
    if (!parseUint32(input["nominalResistanceOhms"], nominalResistance)) {
        error = "ntc thermistor nominal resistance must be numeric";
        return false;
    }
    nominalResistanceOhms = nominalResistance;

    int16_t nominalTemp = nominalTempCentiCelsius;
    if (!parseNominalTemp(input, nominalTemp)) {
        error = "ntc thermistor nominal temperature must be numeric";
        return false;
    }
    nominalTempCentiCelsius = nominalTemp;

    uint16_t beta = betaCoefficient;
    if (!parseUint16(input["betaCoefficient"], beta)) {
        error = "ntc thermistor beta coefficient must be numeric";
        return false;
    }
    betaCoefficient = beta;

    if (!parseFloat(input["steinhartA"], steinhartA) || !parseFloat(input["steinhartB"], steinhartB) ||
        !parseFloat(input["steinhartC"], steinhartC)) {
        error = "ntc thermistor steinhart-hart coefficients must be numeric";
        return false;
    }

    TemperatureUnit unit{};
    if (!temperatureUnitFromString(input["unit"] | temperatureUnitName(static_cast<TemperatureUnit>(outputUnit)), unit)) {
        error = "ntc thermistor output unit is invalid";
        return false;
    }
    outputUnit = temperatureUnitToByte(unit);

    uint32_t poll = pollMs;
    if (!parseUint32(input["pollMs"], poll)) {
        error = "ntc thermistor poll period must be numeric";
        return false;
    }
    pollMs = poll;

    uint16_t reportDelta = reportDeltaCentiCelsius;
    if (!parseReportDelta(input, reportDelta)) {
        error = "ntc thermistor report delta must be numeric";
        return false;
    }
    reportDeltaCentiCelsius = reportDelta;

    if (!filter.parseJson(input, error)) {
        return false;
    }

    const DeviceValidationResult result = validate();
    if (!result.ok()) {
        error = result.message;
        return false;
    }
    return true;
}

void NtcThermistorTemperatureSensorConfigV1::writeJson(JsonObject output) const {
    DeviceBaseConfigV1::writeJson(output);
    NtcFormulaMode parsedMode{};
    (void)ntcFormulaModeFromByte(formulaMode, parsedMode);
    output["formulaMode"] = ntcFormulaModeName(parsedMode);
    output["seriesResistorOhms"] = seriesResistorOhms;
    output["supplyMilliVolts"] = supplyMilliVolts;
    output["nominalResistanceOhms"] = nominalResistanceOhms;
    output["nominalTempCelsius"] = static_cast<float>(nominalTempCentiCelsius) / 100.0F;
    output["nominalTempCentiCelsius"] = nominalTempCentiCelsius;
    output["betaCoefficient"] = betaCoefficient;
    output["steinhartA"] = steinhartA;
    output["steinhartB"] = steinhartB;
    output["steinhartC"] = steinhartC;
    output["unit"] = temperatureUnitName(static_cast<TemperatureUnit>(outputUnit));
    output["pollMs"] = pollMs;
    output["reportDeltaCelsius"] = static_cast<float>(reportDeltaCentiCelsius) / 100.0F;
    output["reportDeltaCentiCelsius"] = reportDeltaCentiCelsius;
    output["reportAlways"] = reportAlways != 0U;
    filter.writeJson(output);
}

} // namespace ewfm
