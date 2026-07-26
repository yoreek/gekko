#include "integrations/rest/aht10/Aht10SensorDeviceApiAdapter.h"

#include "integrations/rest/common/I2cDeviceApiSupport.h"

namespace ewfm {
namespace {
bool parseDepsField(const JsonObjectConst& input, std::array<DeviceDependencyLink, kMaxDeviceDependencies>& deps, uint8_t& depCount,
                    const char*& error) {
    return IDeviceApiAdapter::parseDependenciesJson(input, deps, depCount, error);
}

} // namespace

bool Aht10SensorDeviceApiAdapter::parseCreateExtras(const JsonObjectConst& input, const JsonObjectConst& configInput,
                                                    Aht10SensorConfigV1& config, DeviceCreateRequest& request, const char*& error) const {
    (void)input;
    (void)config;
    return parseDepsField(configInput, request.deps, request.depCount, error);
}

bool Aht10SensorDeviceApiAdapter::parseUpdateExtras(const JsonObjectConst& input, const JsonObjectConst& configInput,
                                                    Aht10SensorConfigV1& config, DeviceConfigUpdateRequest& request,
                                                    const char*& error) const {
    (void)configInput;
    (void)config;
    request.depsProvided = !input["deps"].isNull();
    if (request.depsProvided && !parseDepsField(input, request.deps, request.depCount, error)) {
        return false;
    }
    return true;
}

DeviceValidationResult Aht10SensorDeviceApiAdapter::validateCreateRequest(const DeviceCreateRequest& request,
                                                                          const DeviceRegistry& registry) const {
    Aht10SensorConfigV1 config{};
    if (!decodeAht10SensorConfig(reinterpret_cast<const uint8_t*>(request.configBlob.data()), request.configBlob.size(), config)) {
        return {DeviceError::InvalidConfig, kInvalidConfigError};
    }
    return validateI2cBusDependency(registry, i2cBusDependencyId(request.deps.data(), request.depCount), config.i2cAddress, nullptr);
}

DeviceValidationResult Aht10SensorDeviceApiAdapter::validateUpdateConfigRequest(const IDeviceRuntime& runtime,
                                                                                const DeviceConfigUpdateRequest& request,
                                                                                const DeviceRegistry& registry) const {
    Aht10SensorConfigV1 config{};
    if (!decodeAht10SensorConfig(reinterpret_cast<const uint8_t*>(request.configBlob.data()), request.configBlob.size(), config)) {
        return {DeviceError::InvalidConfig, kInvalidConfigError};
    }

    const DeviceId dependencyDeviceId =
        request.depsProvided ? i2cBusDependencyId(request.deps.data(), request.depCount) : runtime.dependencyDeviceId(DeviceRole::I2CBus);
    return validateI2cBusDependency(registry, dependencyDeviceId, config.i2cAddress, &runtime);
}

DeviceValidationResult
Aht10SensorDeviceApiAdapter::validateSetDepsRequest(const IDeviceRuntime& runtime,
                                                    const std::array<DeviceDependencyLink, kMaxDeviceDependencies>& deps, uint8_t depCount,
                                                    const DeviceRegistry& registry) const {
    uint8_t address = 0;
    if (!runtime.i2cAddress(address)) {
        return {DeviceError::InvalidConfig, kInvalidConfigError};
    }
    return validateI2cBusDependency(registry, i2cBusDependencyId(deps.data(), depCount), address, &runtime);
}

void Aht10SensorDeviceApiAdapter::writeRuntimeJson(const Aht10SensorDevice& device, JsonObject runtimeJson) const {
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
