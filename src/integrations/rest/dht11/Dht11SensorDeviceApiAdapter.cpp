#include "integrations/rest/dht11/Dht11SensorDeviceApiAdapter.h"

namespace ewfm {
namespace {
bool parseNoDeps(const JsonObjectConst& input, const char*& error) {
    if (input.containsKey("deps")) {
        const JsonArrayConst deps = input["deps"].as<JsonArrayConst>();
        if (deps.isNull() || deps.size() > 0U) {
            error = "dht11 does not use dependencies";
            return false;
        }
    }
    error = nullptr;
    return true;
}
} // namespace

bool Dht11SensorDeviceApiAdapter::parseCreateExtras(const JsonObjectConst& input, const JsonObjectConst& configInput,
                                                    Dht11SensorConfigV2& config, DeviceCreateRequest& request, const char*& error) const {
    (void)configInput;
    (void)config;
    request.depCount = 0U;
    request.deps = {};
    return parseNoDeps(input, error);
}

bool Dht11SensorDeviceApiAdapter::parseUpdateExtras(const JsonObjectConst& input, const JsonObjectConst& configInput,
                                                    Dht11SensorConfigV2& config, DeviceConfigUpdateRequest& request,
                                                    const char*& error) const {
    (void)configInput;
    (void)config;
    request.depsProvided = false;
    request.depCount = 0U;
    request.deps = {};
    return parseNoDeps(input, error);
}

DeviceValidationResult Dht11SensorDeviceApiAdapter::validateCreateRequest(const DeviceCreateRequest& request,
                                                                          const DeviceRegistry& registry) const {
    (void)registry;
    Dht11SensorConfigV2 config{};
    if (!decodeDht11SensorConfig(reinterpret_cast<const uint8_t*>(request.configBlob.data()), request.configBlob.size(), config)) {
        return {DeviceError::InvalidConfig, kInvalidConfigError};
    }
    return config.validate();
}

DeviceValidationResult Dht11SensorDeviceApiAdapter::validateUpdateConfigRequest(const IDeviceRuntime& runtime,
                                                                                const DeviceConfigUpdateRequest& request,
                                                                                const DeviceRegistry& registry) const {
    (void)runtime;
    (void)registry;
    Dht11SensorConfigV2 config{};
    if (!decodeDht11SensorConfig(reinterpret_cast<const uint8_t*>(request.configBlob.data()), request.configBlob.size(), config)) {
        return {DeviceError::InvalidConfig, kInvalidConfigError};
    }
    return config.validate();
}

DeviceValidationResult
Dht11SensorDeviceApiAdapter::validateSetDepsRequest(const IDeviceRuntime& runtime,
                                                    const std::array<DeviceDependencyLink, kMaxDeviceDependencies>& deps, uint8_t depCount,
                                                    const DeviceRegistry& registry) const {
    (void)runtime;
    (void)registry;
    if (depCount != 0U) {
        (void)deps;
        return {DeviceError::InvalidRelationship, "dht11 does not use dependencies"};
    }
    return {};
}

void Dht11SensorDeviceApiAdapter::writeRuntimeJson(const Dht11SensorDevice& device, JsonObject runtimeJson) const {
    JsonObject outputJson = runtimeJson.createNestedObject("output");

    TemperatureReading temperatureReading{};
    (void)device.latestTemperatureReading(temperatureReading);
    JsonObject temperatureJson = outputJson.createNestedObject("temperature");
    writeTemperatureOutputJson(temperatureReading,
                               device.config().outputUnit == temperatureUnitToByte(TemperatureUnit::Fahrenheit)
                                   ? TemperatureUnit::Fahrenheit
                                   : TemperatureUnit::Celsius,
                               device.latestTemperatureStatus(), temperatureJson);

    HumidityReading humidityReading{};
    (void)device.latestHumidityReading(humidityReading);
    JsonObject humidityJson = outputJson.createNestedObject("humidity");
    writeHumidityOutputJson(humidityReading, device.latestHumidityStatus(), humidityJson);
}

} // namespace ewfm
