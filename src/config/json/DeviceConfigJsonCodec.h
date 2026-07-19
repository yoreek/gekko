#pragma once

#include "config/DeviceConfig.h"

#include <ArduinoJson.h>

namespace ewfm {

class DeviceConfigJsonCodec {
public:
    static void write(JsonDocument& doc, const DeviceConfig& config);
    static void read(JsonDocument& doc, DeviceConfig& config);
};

} // namespace ewfm
