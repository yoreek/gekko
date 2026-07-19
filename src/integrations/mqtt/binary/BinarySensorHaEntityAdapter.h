#pragma once

#include "integrations/mqtt/HaEntityAdapter.h"

namespace ewfm {

class BinarySensorHaEntityAdapter final : public IHaEntityAdapter {
public:
    static const BinarySensorHaEntityAdapter& instance();

    DeviceTypeId typeId() const override;
    const char* typeName() const override;
    const char* haComponent() const override;
    void buildDiscoveryPayload(const IDeviceRuntime& runtime, const std::string& uniqueId, const std::string& effectiveName,
                               const HaTopicBuilder& topicFor, JsonObject output) const override;
    void publishState(const IDeviceRuntime& runtime, const HaTopicBuilder& topicFor, const HaStatePublisher& publish) const override;
};

} // namespace ewfm
