#include "integrations/mqtt/temperature/TemperatureSensorHaEntityAdapter.h"

#include "devices/sensors/temperature/TemperatureSensorTypes.h"
#include "integrations/mqtt/HaDiscoveryConstants.h"
#include "integrations/mqtt/HaDiscoveryEnvelope.h"

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
    return ha::component::kSensor;
}

void TemperatureSensorHaEntityAdapter::buildDiscoveryPayload(const IDeviceRuntime& runtime, const std::string& uniqueId,
                                                             const std::string& effectiveName, const HaTopicBuilder& topicFor,
                                                             JsonObject output) const {
    (void)runtime;
    writeHaEntityIdentity(output, uniqueId, effectiveName);
    output[ha::key::kStateTopic] = topicFor(ha::component::kSensor, ha::topic::kState);
    output[ha::key::kDeviceClass] = "temperature";
    output[ha::key::kUnitOfMeasurement] = "°C";
    output[ha::key::kStateClass] = "measurement";
    output[ha::key::kIcon] = config_.icon;
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
    publish(topicFor(ha::component::kSensor, ha::topic::kState), buffer);
}

} // namespace ewfm
