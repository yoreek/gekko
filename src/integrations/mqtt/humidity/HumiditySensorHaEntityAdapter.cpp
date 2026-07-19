#include "integrations/mqtt/humidity/HumiditySensorHaEntityAdapter.h"

#include "devices/sensors/humidity/HumiditySensorTypes.h"
#include "integrations/mqtt/HaDiscoveryConstants.h"
#include "integrations/mqtt/HaDiscoveryEnvelope.h"

#include <cstdio>

namespace ewfm {

HumiditySensorHaEntityAdapter::HumiditySensorHaEntityAdapter(HumiditySensorHaEntityAdapterConfig config) : config_(config) {}

DeviceTypeId HumiditySensorHaEntityAdapter::typeId() const {
    return config_.typeId;
}

const char* HumiditySensorHaEntityAdapter::typeName() const {
    return config_.typeName;
}

const char* HumiditySensorHaEntityAdapter::haComponent() const {
    return ha::component::kSensor;
}

void HumiditySensorHaEntityAdapter::buildDiscoveryPayload(const IDeviceRuntime& runtime, const std::string& uniqueId,
                                                          const std::string& effectiveName, const HaTopicBuilder& topicFor,
                                                          JsonObject output) const {
    (void)runtime;
    writeHaEntityIdentity(output, uniqueId, effectiveName);
    output[ha::key::kStateTopic] = topicFor("humidity_sensor", ha::topic::kState);
    output[ha::key::kDeviceClass] = "humidity";
    output[ha::key::kUnitOfMeasurement] = "%";
    output[ha::key::kStateClass] = "measurement";
    output[ha::key::kIcon] = config_.icon;
}

void HumiditySensorHaEntityAdapter::publishState(const IDeviceRuntime& runtime, const HaTopicBuilder& topicFor,
                                                 const HaStatePublisher& publish) const {
    const IHumidityReadingRuntime* humidityRuntime = runtime.humidityReadingRuntime();
    if (humidityRuntime == nullptr) {
        return;
    }

    HumidityReading reading{};
    if (!humidityRuntime->latestHumidityReading(reading) || !reading.valid) {
        return;
    }

    char buffer[16];
    std::snprintf(buffer, sizeof(buffer), "%.2f", static_cast<double>(reading.milliPercent) / 1000.0);
    publish(topicFor("humidity_sensor", ha::topic::kState), buffer);
}

} // namespace ewfm
