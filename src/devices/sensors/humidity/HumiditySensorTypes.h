#pragma once

#include <ArduinoJson.h>
#include <cstdint>

namespace ewfm {

struct HumidityReading {
    int32_t milliPercent{0};
    uint32_t measuredAtMs{0};
    bool valid{false};
};

bool humidityReadingChanged(const HumidityReading& previous, const HumidityReading& current, uint16_t reportDeltaCentiPercent);
void writeHumidityOutputJson(const HumidityReading& reading, const char* status, JsonObject output);

} // namespace ewfm
