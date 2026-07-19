#pragma once

#include "integrations/mqtt/HaEntityAdapter.h"

namespace ewfm {

struct TemperatureSensorHaEntityAdapterConfig {
    DeviceTypeId typeId{0};
    const char* typeName{""};
    const char* icon{"mdi:thermometer"};
};

// Generic Home Assistant "sensor" entity adapter for any device implementing
// ITemperatureReadingRuntime. Discovery/state logic is identical across temperature-sensor
// device families (DS18B20, NTC thermistor, ...) - only typeId/typeName/icon differ, so one
// instance is registered per device type instead of duplicating a whole adapter class.
class TemperatureSensorHaEntityAdapter final : public IHaEntityAdapter {
public:
    explicit TemperatureSensorHaEntityAdapter(TemperatureSensorHaEntityAdapterConfig config);

    DeviceTypeId typeId() const override;
    const char* typeName() const override;
    const char* haComponent() const override;
    void buildDiscoveryPayload(const IDeviceRuntime& runtime, const std::string& uniqueId, const std::string& effectiveName,
                               const HaTopicBuilder& topicFor, JsonObject output) const override;
    void publishState(const IDeviceRuntime& runtime, const HaTopicBuilder& topicFor, const HaStatePublisher& publish) const override;

private:
    TemperatureSensorHaEntityAdapterConfig config_;
};

} // namespace ewfm
