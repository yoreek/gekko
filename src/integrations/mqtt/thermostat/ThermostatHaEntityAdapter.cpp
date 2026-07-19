#include "integrations/mqtt/thermostat/ThermostatHaEntityAdapter.h"

#include "devices/registry/DeviceRegistry.h"
#include "devices/thermostat/ThermostatDevice.h"
#include "integrations/mqtt/HaDiscoveryConstants.h"
#include "integrations/mqtt/HaDiscoveryEnvelope.h"

#include <cstdio>
#include <cstdlib>

namespace ewfm {

namespace {
const char* actionForState(ThermostatMode mode, const ISwitchOutputRuntime::StateType actualOutputState) {
    if (mode == ThermostatMode::Off) {
        return "off";
    }
    if (actualOutputState) {
        return mode == ThermostatMode::Heat ? "heating" : "cooling";
    }
    return "idle";
}
} // namespace

const ThermostatHaEntityAdapter& ThermostatHaEntityAdapter::instance() {
    static const ThermostatHaEntityAdapter adapter;
    return adapter;
}

DeviceTypeId ThermostatHaEntityAdapter::typeId() const {
    return ThermostatDevice::descriptor().typeId;
}

const char* ThermostatHaEntityAdapter::typeName() const {
    return "thermostat";
}

const char* ThermostatHaEntityAdapter::haComponent() const {
    return ha::component::kClimate;
}

void ThermostatHaEntityAdapter::buildDiscoveryPayload(const IDeviceRuntime& runtime, const std::string& uniqueId,
                                                      const std::string& effectiveName, const HaTopicBuilder& topicFor,
                                                      JsonObject output) const {
    const auto& thermostat = static_cast<const ThermostatDevice&>(runtime);
    const ThermostatDeviceConfigV1& config = thermostat.config();

    writeHaEntityIdentity(output, uniqueId, effectiveName);
    output[ha::key::kIcon] = "mdi:thermostat";

    JsonArray modes = output.createNestedArray(ha::key::kModes);
    modes.add("off");
    modes.add("heat");
    modes.add("cool");
    output[ha::key::kModeStateTopic] = topicFor("climate_mode", ha::topic::kState);
    output[ha::key::kModeCommandTopic] = topicFor("climate_mode", ha::topic::kSet);

    output[ha::key::kTemperatureStateTopic] = topicFor("climate_temperature", ha::topic::kState);
    output[ha::key::kTemperatureCommandTopic] = topicFor("climate_temperature", ha::topic::kSet);
    output[ha::key::kCurrentTemperatureTopic] = topicFor("climate_current_temperature", ha::topic::kState);
    output[ha::key::kActionTopic] = topicFor("climate_action", ha::topic::kState);

    output[ha::key::kTemperatureUnit] = "C";
    output[ha::key::kMinTemperature] = static_cast<float>(config.minSafeMilliCelsius) / 1000.0F;
    output[ha::key::kMaxTemperature] = static_cast<float>(config.maxSafeMilliCelsius) / 1000.0F;
    output[ha::key::kTemperatureStep] = 0.5F;
    // Must serialize as exactly 0.1 (not e.g. 0.100000001, which a float literal produces due to
    // binary rounding) - Home Assistant's climate discovery schema only accepts precision values of
    // exactly 0.1, 0.5, or 1.0 and silently rejects the whole discovery payload otherwise.
    output[ha::key::kPrecision] = 0.1;
}

void ThermostatHaEntityAdapter::publishState(const IDeviceRuntime& runtime, const HaTopicBuilder& topicFor,
                                             const HaStatePublisher& publish) const {
    const auto& thermostat = static_cast<const ThermostatDevice&>(runtime);
    const ThermostatDeviceConfigV1& config = thermostat.config();
    const auto mode = static_cast<ThermostatMode>(config.mode);

    publish(topicFor("climate_mode", ha::topic::kState), thermostatModeName(mode));

    char buffer[16];
    std::snprintf(buffer, sizeof(buffer), "%.2f", static_cast<double>(config.targetMilliCelsius) / 1000.0);
    publish(topicFor("climate_temperature", ha::topic::kState), buffer);

    const TemperatureReading& reading = thermostat.latestTemperature();
    if (reading.valid) {
        std::snprintf(buffer, sizeof(buffer), "%.2f", static_cast<double>(reading.milliCelsius) / 1000.0);
        publish(topicFor("climate_current_temperature", ha::topic::kState), buffer);
    }

    publish(topicFor("climate_action", ha::topic::kState), actionForState(mode, thermostat.actualOutputState()));
}

bool ThermostatHaEntityAdapter::applyCommand(DeviceRegistry& registry, const IDeviceRuntime& runtime, DeviceId deviceId,
                                             const std::string& commandKey, const std::string& payload, uint32_t now) const {
    const auto& thermostat = static_cast<const ThermostatDevice&>(runtime);
    ThermostatDeviceConfigV1 config = thermostat.config();

    if (commandKey == "climate_mode") {
        ThermostatMode mode{};
        if (!thermostatModeFromString(payload.c_str(), mode)) {
            return false;
        }
        config.mode = static_cast<uint8_t>(mode);
    } else if (commandKey == "climate_temperature") {
        if (payload.empty()) {
            return false;
        }
        char* end = nullptr;
        const double celsius = std::strtod(payload.c_str(), &end);
        if (end == payload.c_str()) {
            return false;
        }
        config.targetMilliCelsius = static_cast<int32_t>(celsius * 1000.0 + (celsius >= 0.0 ? 0.5 : -0.5));
    } else {
        return false;
    }

    uint8_t buffer[kMaxDeviceConfigBytes]{};
    const size_t size = thermostatDeviceConfigSize(config);
    if (!encodeFixedConfigBlob(ThermostatDeviceConfigV1::kMagic, config, buffer, size)) {
        return false;
    }
    DeviceConfigBlob blob{};
    if (!blob.assign(buffer, size)) {
        return false;
    }
    return registry.updateConfig(deviceId, blob, 0, now).ok();
}

} // namespace ewfm
