#pragma once

#include <cstdint>

namespace ewfm {

enum class NtcFormulaMode : uint8_t {
    Beta = 0,
    SteinhartHart = 1,
};

bool ntcFormulaModeFromByte(uint8_t value, NtcFormulaMode& mode);
bool ntcFormulaModeFromString(const char* value, NtcFormulaMode& mode);
const char* ntcFormulaModeName(NtcFormulaMode mode);

// Voltage divider: a series resistor between Vcc and the ADC node, the NTC between the ADC node
// and GND (Rntc = seriesResistorOhms * Vout / (Vcc - Vout)). Returns false, leaving
// `outResistanceOhms` untouched, when `nodeMilliVolts` is outside the divider's valid range
// (<=0 or >= supplyMilliVolts) -- the caller reports this as an out-of-range reading.
bool ntcDividerResistanceOhms(int32_t nodeMilliVolts, uint16_t seriesResistorOhms, uint16_t supplyMilliVolts, double& outResistanceOhms);

// Beta equation: 1/T = 1/T0 + (1/Beta) * ln(R/R0). Accurate to roughly +-0.5-1 degC over a
// hobbyist/aquarium temperature range for most NTC parts and only needs the two datasheet points
// (R0 at T0, Beta) that every thermistor datasheet publishes. Returns INT32_MIN if the inputs
// don't describe a physically valid curve (non-positive resistance/Beta, or a non-positive
// implied 1/T).
int32_t ntcBetaMilliCelsius(double resistanceOhms, uint32_t nominalResistanceOhms, int16_t nominalTempCentiCelsius,
                            uint16_t betaCoefficient);

// Steinhart-Hart equation: 1/T = A + B*ln(R) + C*ln(R)^3. More accurate than Beta across a wider
// range when the manufacturer publishes (or the user fits from a 3-point R(T) table) A/B/C
// coefficients. Returns INT32_MIN on a non-positive resistance or non-positive implied 1/T.
int32_t ntcSteinhartHartMilliCelsius(double resistanceOhms, float coefficientA, float coefficientB, float coefficientC);

} // namespace ewfm
