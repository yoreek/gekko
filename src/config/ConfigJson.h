#pragma once

#include "config/DeviceConfig.h"

#include <string>

namespace ewfm {

struct JsonResult {
    bool success{false};
    ConfigError error{ConfigError::None};
    const char* message{"ok"};
    std::string payload;
};

JsonResult exportConfigJson(const DeviceConfig& config);
JsonResult importConfigJson(const std::string& json, DeviceConfig& config);

} // namespace ewfm
