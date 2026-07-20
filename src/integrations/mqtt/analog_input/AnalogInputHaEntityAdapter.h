#pragma once

#include "integrations/mqtt/HaEntityAdapter.h"

namespace ewfm {

struct AnalogInputHaEntityAdapterConfig {
    DeviceTypeId typeId{0};
    const char* typeName{""};
    const char* icon{"mdi:flash-outline"};
};

// Generic Home Assistant "sensor" entity adapter for any device implementing
// IAnalogInputRuntime. Discovery/state logic is identical across every AnalogInput backend
// (analog_port_input, ads1115_input, cd74hc4067_input, ...) - only typeId/typeName/icon differ,
// so one instance is registered per leaf type instead of duplicating a whole adapter class.
// Hubs (ads1115_hub, cd74hc4067_hub) are not registered here: they provide channels, not a
// reading of their own.
class AnalogInputHaEntityAdapter final : public IHaEntityAdapter {
public:
    explicit AnalogInputHaEntityAdapter(AnalogInputHaEntityAdapterConfig config);

    DeviceTypeId typeId() const override;
    const char* typeName() const override;
    const char* haComponent() const override;
    void buildDiscoveryPayload(const IDeviceRuntime& runtime, const std::string& uniqueId, const std::string& effectiveName,
                               const HaTopicBuilder& topicFor, JsonObject output) const override;
    void publishState(const IDeviceRuntime& runtime, const HaTopicBuilder& topicFor, const HaStatePublisher& publish) const override;

private:
    AnalogInputHaEntityAdapterConfig config_;
};

} // namespace ewfm
