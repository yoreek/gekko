#include "integrations/mqtt/temperature/TemperatureSensorHaEntityAdapter.h"

#include "devices/sensors/temperature/TemperatureSensorTypes.h"

#include <cstdio>

namespace ewfm {

TemperatureSensorHaEntityAdapter::TemperatureSensorHaEntityAdapter(TemperatureSensorHaEntityAdapterConfig config) : config_(config) {}

DeviceTypeId TemperatureSensorHaEntityAdapter::typeId() const {
    return config_.typeId;
}

const char* TemperatureSensorHaEntityAdapter::typeName() const {
    return config_.typeName;
}

const char* TemperatureSensorHaEntityAdapter::haComponent() const {
    return "sensor";
}

void TemperatureSensorHaEntityAdapter::buildDiscoveryPayload(const IDeviceRuntime& runtime, const std::string& uniqueId,
                                                             const std::string& effectiveName, const HaTopicBuilder& topicFor,
                                                             JsonObject output) const {
    (void)runtime;
    output["unique_id"] = uniqueId;
    output["object_id"] = uniqueId;
    output["name"] = effectiveName;
    output["state_topic"] = topicFor("sensor", "state");
    output["device_class"] = "temperature";
    output["unit_of_measurement"] = "°C";
    output["state_class"] = "measurement";
    output["icon"] = config_.icon;
}

void TemperatureSensorHaEntityAdapter::publishState(const IDeviceRuntime& runtime, const HaTopicBuilder& topicFor,
                                                    const HaStatePublisher& publish) const {
    const ITemperatureReadingRuntime* temperatureRuntime = runtime.temperatureReadingRuntime();
    if (temperatureRuntime == nullptr) {
        return;
    }

    TemperatureReading reading{};
    if (!temperatureRuntime->latestTemperatureReading(reading) || !reading.valid) {
        return;
    }

    // Always published in Celsius regardless of the device's configured display
    // unit (that setting only affects the portal UI/OLED output) - Home Assistant
    // converts to the user's preferred unit on its own for device_class: temperature.
    char buffer[16];
    std::snprintf(buffer, sizeof(buffer), "%.2f", static_cast<double>(reading.milliCelsius) / 1000.0);
    publish(topicFor("sensor", "state"), buffer);
}

bool TemperatureSensorHaEntityAdapter::applyCommand(DeviceRegistry& registry, const IDeviceRuntime& runtime, DeviceId deviceId,
                                                    const std::string& commandKey, const std::string& payload, uint32_t now) const {
    (void)registry;
    (void)runtime;
    (void)deviceId;
    (void)commandKey;
    (void)payload;
    (void)now;
    return false; // read-only sensor: no commands accepted
}

} // namespace ewfm
