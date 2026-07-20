#include "integrations/mqtt/analog_input/AnalogInputHaEntityAdapter.h"

#include "integrations/mqtt/HaDiscoveryConstants.h"
#include "integrations/mqtt/HaDiscoveryEnvelope.h"

#include <cstdio>

namespace ewfm {

AnalogInputHaEntityAdapter::AnalogInputHaEntityAdapter(AnalogInputHaEntityAdapterConfig config) : config_(config) {}

DeviceTypeId AnalogInputHaEntityAdapter::typeId() const {
    return config_.typeId;
}

const char* AnalogInputHaEntityAdapter::typeName() const {
    return config_.typeName;
}

const char* AnalogInputHaEntityAdapter::haComponent() const {
    return ha::component::kSensor;
}

void AnalogInputHaEntityAdapter::buildDiscoveryPayload(const IDeviceRuntime& runtime, const std::string& uniqueId,
                                                       const std::string& effectiveName, const HaTopicBuilder& topicFor,
                                                       JsonObject output) const {
    (void)runtime;
    writeHaEntityIdentity(output, uniqueId, effectiveName);
    output[ha::key::kStateTopic] = topicFor(ha::component::kSensor, ha::topic::kState);
    output[ha::key::kDeviceClass] = "voltage";
    output[ha::key::kUnitOfMeasurement] = "V";
    output[ha::key::kStateClass] = "measurement";
    output[ha::key::kIcon] = config_.icon;
}

void AnalogInputHaEntityAdapter::publishState(const IDeviceRuntime& runtime, const HaTopicBuilder& topicFor,
                                              const HaStatePublisher& publish) const {
    const IAnalogInputRuntime* analogInputRuntime = runtime.analogInputRuntime();
    if (analogInputRuntime == nullptr) {
        return;
    }

    AnalogInputReading reading{};
    if (!analogInputRuntime->latestAnalogInputReading(reading) || !reading.valid) {
        return;
    }

    char buffer[16];
    std::snprintf(buffer, sizeof(buffer), "%.3f", static_cast<double>(reading.milliVolts) / 1000.0);
    publish(topicFor(ha::component::kSensor, ha::topic::kState), buffer);
}

} // namespace ewfm
