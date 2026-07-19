#pragma once

#include "config/DeviceConfig.h"

#include <ArduinoJson.h>

namespace ewfm {

class WifiConfigJson {
public:
    static void write(JsonDocument& doc, const WiFiCredentials& credentials);
    static void read(JsonDocument& doc, DeviceConfig& config);
};

} // namespace ewfm
