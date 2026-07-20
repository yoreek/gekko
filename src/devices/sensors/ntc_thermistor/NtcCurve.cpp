#include "devices/sensors/ntc_thermistor/NtcCurve.h"

#include <cmath>
#include <cstdint>
#include <cstring>

namespace ewfm {

namespace {
constexpr double kKelvinOffset = 273.15;
} // namespace

bool ntcFormulaModeFromByte(uint8_t value, NtcFormulaMode& mode) {
    if (value > static_cast<uint8_t>(NtcFormulaMode::SteinhartHart)) {
        return false;
    }
    mode = static_cast<NtcFormulaMode>(value);
    return true;
}

bool ntcFormulaModeFromString(const char* value, NtcFormulaMode& mode) {
    if (value == nullptr) {
        return false;
    }
    if (std::strcmp(value, "beta") == 0) {
        mode = NtcFormulaMode::Beta;
        return true;
    }
    if (std::strcmp(value, "steinhart_hart") == 0) {
        mode = NtcFormulaMode::SteinhartHart;
        return true;
    }
    return false;
}

const char* ntcFormulaModeName(NtcFormulaMode mode) {
    switch (mode) {
    case NtcFormulaMode::Beta:
        return "beta";
    case NtcFormulaMode::SteinhartHart:
        return "steinhart_hart";
    }
    return "beta";
}

bool ntcDividerResistanceOhms(int32_t nodeMilliVolts, uint16_t seriesResistorOhms, uint16_t supplyMilliVolts, double& outResistanceOhms) {
    if (nodeMilliVolts <= 0 || nodeMilliVolts >= static_cast<int32_t>(supplyMilliVolts)) {
        return false;
    }
    const double vOut = static_cast<double>(nodeMilliVolts);
    const double vSupply = static_cast<double>(supplyMilliVolts);
    outResistanceOhms = static_cast<double>(seriesResistorOhms) * vOut / (vSupply - vOut);
    return true;
}

int32_t ntcBetaMilliCelsius(double resistanceOhms, uint32_t nominalResistanceOhms, int16_t nominalTempCentiCelsius,
                            uint16_t betaCoefficient) {
    if (resistanceOhms <= 0.0 || nominalResistanceOhms == 0U || betaCoefficient == 0U) {
        return INT32_MIN;
    }
    const double nominalTempKelvin = static_cast<double>(nominalTempCentiCelsius) / 100.0 + kKelvinOffset;
    const double inverseT = 1.0 / nominalTempKelvin + (1.0 / static_cast<double>(betaCoefficient)) *
                                                          std::log(resistanceOhms / static_cast<double>(nominalResistanceOhms));
    if (inverseT <= 0.0) {
        return INT32_MIN;
    }
    const double tempCelsius = 1.0 / inverseT - kKelvinOffset;
    return static_cast<int32_t>(std::lround(tempCelsius * 1000.0));
}

int32_t ntcSteinhartHartMilliCelsius(double resistanceOhms, float coefficientA, float coefficientB, float coefficientC) {
    if (resistanceOhms <= 0.0) {
        return INT32_MIN;
    }
    const double lnR = std::log(resistanceOhms);
    const double inverseT =
        static_cast<double>(coefficientA) + static_cast<double>(coefficientB) * lnR + static_cast<double>(coefficientC) * lnR * lnR * lnR;
    if (inverseT <= 0.0) {
        return INT32_MIN;
    }
    const double tempCelsius = 1.0 / inverseT - kKelvinOffset;
    return static_cast<int32_t>(std::lround(tempCelsius * 1000.0));
}

} // namespace ewfm
