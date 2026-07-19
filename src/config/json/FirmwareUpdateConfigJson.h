#pragma once

#include "config/DeviceConfig.h"

#include <ArduinoJson.h>

namespace ewfm {

class FirmwareUpdateConfigJson {
public:
    static void write(JsonDocument& doc, const FirmwareUpdateConfig& config);
    static void read(JsonDocument& doc, DeviceConfig& config);
};

} // namespace ewfm
