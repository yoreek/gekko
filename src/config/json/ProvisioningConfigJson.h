#pragma once

#include "config/DeviceConfig.h"

#include <ArduinoJson.h>

namespace ewfm {

class ProvisioningConfigJson {
public:
    static void write(JsonDocument& doc, const ProvisioningConfig& config);
    static void read(JsonDocument& doc, DeviceConfig& config);
};

} // namespace ewfm
