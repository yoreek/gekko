#include "integrations/rest/ntc_thermistor/NtcThermistorTemperatureSensorDeviceApiAdapter.h"

#include "integrations/rest/common/AnalogInputDeviceApiSupport.h"

namespace ewfm {
namespace {
bool parseDepsField(const JsonObjectConst& input, std::array<DeviceDependencyLink, kMaxDeviceDependencies>& deps, uint8_t& depCount,
                    const char*& error) {
    return IDeviceApiAdapter::parseDependenciesJson(input, deps, depCount, error);
}
} // namespace

bool NtcThermistorTemperatureSensorDeviceApiAdapter::parseCreateExtras(const JsonObjectConst& input, const JsonObjectConst& configInput,
                                                                       NtcThermistorTemperatureSensorConfigV1& config,
                                                                       DeviceCreateRequest& request, const char*& error) const {
    (void)input;
    (void)config;
    return parseDepsField(configInput, request.deps, request.depCount, error);
}

bool NtcThermistorTemperatureSensorDeviceApiAdapter::parseUpdateExtras(const JsonObjectConst& input, const JsonObjectConst& configInput,
                                                                       NtcThermistorTemperatureSensorConfigV1& config,
                                                                       DeviceConfigUpdateRequest& request, const char*& error) const {
    (void)configInput;
    (void)config;
    request.depsProvided = !input["deps"].isNull();
    if (request.depsProvided && !parseDepsField(input, request.deps, request.depCount, error)) {
        return false;
    }
    return true;
}

DeviceValidationResult NtcThermistorTemperatureSensorDeviceApiAdapter::validateCreateRequest(const DeviceCreateRequest& request,
                                                                                             const DeviceRegistry& registry) const {
    return validateAnalogInputDependency(registry, analogInputDependencyId(request.deps.data(), request.depCount));
}

DeviceValidationResult NtcThermistorTemperatureSensorDeviceApiAdapter::validateUpdateConfigRequest(const IDeviceRuntime& runtime,
                                                                                                   const DeviceConfigUpdateRequest& request,
                                                                                                   const DeviceRegistry& registry) const {
    const DeviceId dependencyDeviceId = request.depsProvided ? analogInputDependencyId(request.deps.data(), request.depCount)
                                                             : runtime.dependencyDeviceId(DeviceRole::AnalogInput);
    return validateAnalogInputDependency(registry, dependencyDeviceId);
}

DeviceValidationResult
NtcThermistorTemperatureSensorDeviceApiAdapter::validateSetDepsRequest(const IDeviceRuntime& runtime,
                                                                       const std::array<DeviceDependencyLink, kMaxDeviceDependencies>& deps,
                                                                       uint8_t depCount, const DeviceRegistry& registry) const {
    (void)runtime;
    return validateAnalogInputDependency(registry, analogInputDependencyId(deps.data(), depCount));
}

void NtcThermistorTemperatureSensorDeviceApiAdapter::writeRuntimeJson(const NtcThermistorTemperatureSensorDevice& device,
                                                                      JsonObject runtimeJson) const {
    JsonObject outputJson = runtimeJson.createNestedObject("output");
    JsonObject temperature = outputJson.createNestedObject("temperature");
    writeTemperatureOutputJson(device.reading(),
                               device.config().outputUnit == temperatureUnitToByte(TemperatureUnit::Fahrenheit)
                                   ? TemperatureUnit::Fahrenheit
                                   : TemperatureUnit::Celsius,
                               device.outputStatus(), temperature);
}

} // namespace ewfm
