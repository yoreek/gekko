#pragma once

#include "integrations/mqtt/HaEntityAdapter.h"

namespace ewfm {

class Ds18b20HaEntityAdapter final : public IHaEntityAdapter {
public:
    static const Ds18b20HaEntityAdapter& instance();

    DeviceTypeId typeId() const override;
    const char* typeName() const override;
    const char* haComponent() const override;
    void buildDiscoveryPayload(const IDeviceRuntime& runtime, const std::string& uniqueId, const std::string& effectiveName,
                               const HaTopicBuilder& topicFor, JsonObject output) const override;
    void publishState(const IDeviceRuntime& runtime, const HaTopicBuilder& topicFor, const HaStatePublisher& publish) const override;
    bool applyCommand(DeviceRegistry& registry, const IDeviceRuntime& runtime, DeviceId deviceId, const std::string& commandKey,
                      const std::string& payload, uint32_t now) const override;
};

} // namespace ewfm
