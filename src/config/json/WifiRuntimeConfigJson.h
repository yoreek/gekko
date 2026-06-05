#pragma once

#include "config/DeviceConfig.h"

#include <ArduinoJson.h>

namespace ewfm {

class WifiRuntimeConfigJson {
public:
    static void write(JsonDocument& doc, const WifiRuntimeConfig& config);
    static void read(JsonDocument& doc, DeviceConfig& config);
};

} // namespace ewfm
