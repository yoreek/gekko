#pragma once

#include "integrations/mqtt/HaEntityAdapter.h"

namespace ewfm {

// One dosing pump maps to five HA entities (four monitoring + the auto-mode switch). Deliberately
// no dose start/stop from HA - dosing is chemistry, misfires are costly, so run control stays in
// the portal UI. Instead of five near-identical adapter classes (flash budget), one class is
// parameterized by Kind, mirroring how TemperatureSensorHaEntityAdapter is instantiated per type.
enum class DosingPumpHaEntityKind : uint8_t {
    RunState,       // sensor: enum idle/dosing
    TodayDosed,     // sensor: mL dosed since local midnight
    ContainerLevel, // sensor: mL remaining in the container
    ContainerEmpty, // binary_sensor: device_class problem
    AutoMode,       // switch: ON = schedule runs, OFF = manual only
};

struct DosingPumpHaEntityAdapterConfig {
    DosingPumpHaEntityKind kind{DosingPumpHaEntityKind::RunState};
    const char* typeName{""};    // unique_id segment, distinct per entity
    const char* channel{""};     // topic channel / commandKey, distinct per entity
    const char* haComponent{""}; // "sensor" | "binary_sensor" | "switch"
    const char* nameSuffix{""};  // appended to the device name so the five entities stay tellable apart
    const char* icon{""};
};

class DosingPumpHaEntityAdapter final : public IHaEntityAdapter {
public:
    explicit DosingPumpHaEntityAdapter(DosingPumpHaEntityAdapterConfig config);

    DeviceTypeId typeId() const override;
    const char* typeName() const override;
    const char* haComponent() const override;
    void buildDiscoveryPayload(const IDeviceRuntime& runtime, const std::string& uniqueId, const std::string& effectiveName,
                               const HaTopicBuilder& topicFor, JsonObject output) const override;
    void publishState(const IDeviceRuntime& runtime, const HaTopicBuilder& topicFor, const HaStatePublisher& publish) const override;
    bool applyCommand(DeviceRegistry& registry, const IDeviceRuntime& runtime, DeviceId deviceId, const std::string& commandKey,
                      const std::string& payload, uint32_t now) const override;

private:
    DosingPumpHaEntityAdapterConfig config_;
};

} // namespace ewfm
