#include "integrations/rest/ntc_thermistor/NtcThermistorTemperatureSensorDeviceApiAdapter.h"

#include "devices/core/DeviceBaseConfig.h"
#include "devices/sensors/ntc_thermistor/NtcThermistorTemperatureSensorDevice.h"

namespace ewfm {

const NtcThermistorTemperatureSensorDeviceApiAdapter& NtcThermistorTemperatureSensorDeviceApiAdapter::instance() {
    static const NtcThermistorTemperatureSensorDeviceApiAdapter adapter;
    return adapter;
}

DeviceTypeId NtcThermistorTemperatureSensorDeviceApiAdapter::typeId() const {
    return kNtcThermistorTemperatureSensorTypeId;
}

const char* NtcThermistorTemperatureSensorDeviceApiAdapter::typeName() const {
    return "ntc_thermistor_temperature_sensor";
}

bool NtcThermistorTemperatureSensorDeviceApiAdapter::parseCreateRequest(const JsonObjectConst& input, DeviceCreateRequest& request,
                                                                        const char*& error) const {
    request = {};
    request.typeId = typeId();
    request.configVersion = kNtcThermistorTemperatureSensorConfigVersion;

    const JsonObjectConst configInput = input["config"].as<JsonObjectConst>();
    if (configInput.isNull()) {
        error = "ntc thermistor config is required";
        return false;
    }

    NtcThermistorTemperatureSensorConfigV1 config{};
    if (!config.parseJson(configInput, error)) {
        return false;
    }
    request.name = config.name;
    request.enabled = config.enabled != 0U;

    if (!config.validate().ok()) {
        error = "ntc thermistor config is invalid";
        return false;
    }
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    const size_t size = ntcThermistorTemperatureSensorConfigSize(config);
    if (!encodeNtcThermistorTemperatureSensorConfig(config, buffer, size) || !request.configBlob.assign(buffer, size)) {
        error = "failed to encode ntc thermistor config";
        return false;
    }
    return true;
}

bool NtcThermistorTemperatureSensorDeviceApiAdapter::parseUpdateConfigRequest(const JsonObjectConst& input, IDeviceRuntime& runtime,
                                                                              DeviceConfigUpdateRequest& request,
                                                                              const char*& error) const {
    const JsonObjectConst configInput = input["config"].as<JsonObjectConst>();
    if (configInput.isNull()) {
        error = "ntc thermistor config is required";
        return false;
    }

    NtcThermistorTemperatureSensorConfigV1 config = static_cast<const NtcThermistorTemperatureSensorDevice&>(runtime).config();
    if (!config.parseJson(configInput, error)) {
        return false;
    }
    if (!config.validate().ok()) {
        error = "ntc thermistor config is invalid";
        return false;
    }

    request = {};
    request.configVersion = kNtcThermistorTemperatureSensorConfigVersion;
    request.enabled = config.enabled != 0U;
    if (!copyBoundedText(request.name, config.name)) {
        error = "device base config is invalid";
        return false;
    }
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    const size_t size = ntcThermistorTemperatureSensorConfigSize(config);
    if (!encodeNtcThermistorTemperatureSensorConfig(config, buffer, size) || !request.configBlob.assign(buffer, size)) {
        error = "failed to encode ntc thermistor config";
        return false;
    }
    return true;
}

void NtcThermistorTemperatureSensorDeviceApiAdapter::writeDeviceJson(const IDeviceRuntime& runtime, const DeviceStatus effectiveStatus,
                                                                     JsonObject output) const {
    writeCommonDeviceJson(runtime, effectiveStatus, typeName(), output);
    const NtcThermistorTemperatureSensorDevice& device = static_cast<const NtcThermistorTemperatureSensorDevice&>(runtime);
    JsonObject config = output["config"].as<JsonObject>();
    device.config().writeJson(config);

    JsonObject runtimeJson = output["runtime"].as<JsonObject>();
    JsonObject outputJson = runtimeJson.createNestedObject("output");
    JsonObject temperature = outputJson.createNestedObject("temperature");
    writeTemperatureOutputJson(device.reading(),
                               device.config().outputUnit == temperatureUnitToByte(TemperatureUnit::Fahrenheit)
                                   ? TemperatureUnit::Fahrenheit
                                   : TemperatureUnit::Celsius,
                               device.outputStatus(), temperature);
}

} // namespace ewfm
