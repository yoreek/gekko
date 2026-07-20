#include "devices/analog/input/AnalogInputJson.h"

namespace ewfm {

void writeAnalogInputOutputJson(const AnalogInputReading& reading, const char* status, JsonObject output) {
    output["milliVolts"] = reading.valid ? reading.milliVolts : 0;
    output["rawCode"] = reading.valid ? reading.rawCode : 0U;
    output["measuredAtMs"] = reading.valid ? reading.measuredAtMs : 0U;
    output["valid"] = reading.valid;
    output["status"] = status != nullptr ? status : (reading.valid ? "ok" : "not_ready");
}

} // namespace ewfm
