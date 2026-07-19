#include "devices/sensors/humidity/HumiditySensorTypes.h"

#include <cstdlib>

namespace ewfm {

bool humidityReadingChanged(const HumidityReading& previous, const HumidityReading& current, uint16_t reportDeltaCentiPercent) {
    if (previous.valid != current.valid) {
        return true;
    }
    if (!current.valid) {
        return false;
    }

    const int32_t deltaMilliPercent = std::abs(current.milliPercent - previous.milliPercent);
    return deltaMilliPercent >= static_cast<int32_t>(reportDeltaCentiPercent) * 10;
}

void writeHumidityOutputJson(const HumidityReading& reading, const char* status, JsonObject output) {
    output["value"] = reading.valid ? static_cast<float>(reading.milliPercent) / 1000.0F : 0.0F;
    output["unitSymbol"] = "%";
    output["measuredAtMs"] = reading.valid ? reading.measuredAtMs : 0U;
    output["valid"] = reading.valid;
    output["status"] = status != nullptr ? status : (reading.valid ? "ok" : "not_ready");
}

} // namespace ewfm
