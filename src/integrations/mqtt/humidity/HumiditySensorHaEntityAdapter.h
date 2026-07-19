#pragma once

#include "integrations/mqtt/HaEntityAdapter.h"

namespace ewfm {

struct HumiditySensorHaEntityAdapterConfig {
    DeviceTypeId typeId{0};
    const char* typeName{""};
    const char* icon{"mdi:water-percent"};
};

// Generic Home Assistant "sensor" entity adapter for any device implementing
// IHumidityReadingRuntime. Mirrors TemperatureSensorHaEntityAdapter - one instance is registered
// per device type/channel combination instead of duplicating a whole adapter class. A device that
// exposes both temperature and humidity (e.g. HTU21) registers one of each adapter for its typeId.
class HumiditySensorHaEntityAdapter final : public IHaEntityAdapter {
public:
    explicit HumiditySensorHaEntityAdapter(HumiditySensorHaEntityAdapterConfig config);

    DeviceTypeId typeId() const override;
    const char* typeName() const override;
    const char* haComponent() const override;
    void buildDiscoveryPayload(const IDeviceRuntime& runtime, const std::string& uniqueId, const std::string& effectiveName,
                               const HaTopicBuilder& topicFor, JsonObject output) const override;
    void publishState(const IDeviceRuntime& runtime, const HaTopicBuilder& topicFor, const HaStatePublisher& publish) const override;

private:
    HumiditySensorHaEntityAdapterConfig config_;
};

} // namespace ewfm
