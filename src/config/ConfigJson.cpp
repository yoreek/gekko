#include "config/ConfigJson.h"

#include "config/json/DeviceConfigJsonCodec.h"

#include <ArduinoJson.h>

namespace ewfm {

JsonResult exportConfigJson(const DeviceConfig& config) {
    JsonResult result;
    DynamicJsonDocument doc(config.maxJsonBytes);
    DeviceConfigJsonCodec::write(doc, config);

    serializeJson(doc, result.payload);
    result.success = true;
    return result;
}

JsonResult importConfigJson(const std::string& json, DeviceConfig& config) {
    if (json.size() > config.maxJsonBytes) {
        return {false, ConfigError::JsonTooLarge, "json input is too large", {}};
    }

    DynamicJsonDocument doc(config.maxJsonBytes);
    DeserializationError error = deserializeJson(doc, json);
    if (error) {
        return {false, ConfigError::InvalidJson, "invalid json", {}};
    }

    DeviceConfig next = config;
    DeviceConfigJsonCodec::read(doc, next);

    ValidationResult migrated = migrateConfig(next);
    if (!migrated.ok()) {
        return {false, migrated.error, migrated.message, {}};
    }

    config = next;
    return {true, ConfigError::None, "ok", {}};
}

} // namespace ewfm
