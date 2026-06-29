#pragma once

#include "core/StateMachine.h"
#include "devices/core/DeviceTypes.h"

#include <vector>

namespace ewfm {

class DeviceRuntimeBase : public StateMachine, public IDeviceRuntime {
public:
    explicit DeviceRuntimeBase(PState initialState);

    void begin(uint32_t now) override;
    void tickFastLoop(uint32_t now) override;
    void tick100ms(uint32_t now) override;
    void tick1s(uint32_t now) override;
    void end(uint32_t now) override;
    void resetStateMachine(uint32_t now) override;
    void setDependencyRuntime(DeviceDependencyRole role, IDeviceRuntime* dependencyRuntime) override;
    void setDependencyRuntimeAt(uint8_t index, IDeviceRuntime* dependencyRuntime) override;
    IDeviceRuntime* dependencyRuntime(DeviceDependencyRole role) const override;
    IDeviceRuntime* dependencyRuntimeAt(uint8_t index) const override;
    uint8_t dependencyCount() const override;
    const DeviceDependencyLink* dependencyLinks() const override;
    void attachDependentRuntime(IDeviceRuntime* dependentRuntime) override;
    void detachDependentRuntime(IDeviceRuntime* dependentRuntime) override;
    const std::vector<IDeviceRuntime*>& dependentRuntimes() const override;
    void requestReconfigure() override;
    void requestDisable() override;
    void requestDelete() override;
    void clearLifecycleRequests() override;
    DeviceStatus status() const override;
    void bindDeviceIdentity(const DeviceRegistryEntry& record, const DeviceConfigBlob& config) override;
    void writeCommonDeviceJson(JsonObject output) const;
    DeviceId deviceId() const override;
    DeviceTypeId typeId() const override;
    uint32_t configVersion() const override;
    uint32_t configRevision() const override;
    bool hasDependencies() const override;
    DeviceId dependencyDeviceId(DeviceDependencyRole role) const override;
    bool enabled() const override;
    const char* name() const override;
    DevicePersistencePolicy persistencePolicy() const override;
    bool handleCommand(const DeviceCommand& command) override;

#ifdef UNIT_TEST
#endif

protected:
    void tickRuntime(uint32_t now);
    void setStatus(DeviceStatus status);
    bool dependencyReady(DeviceDependencyRole role) const;
    bool dependenciesReady() const;
    bool hasDependentRuntime(const IDeviceRuntime* dependentRuntime) const;
    bool startRequested() const;
    bool reconfigureRequested() const;
    bool disableRequested() const;
    bool deleteRequested() const;
    bool faultRequested() const;
    void requestFault();
    void clearFaultRequested();
    void clearStartRequested();
    void clearReconfigureRequested();
    void clearDisableRequested();
    void clearDeleteRequested();
    void setDeleted();
    void markRuntimeStateDirty();
    bool runtimeStateDirty() const override;
    void clearRuntimeStateDirty() override;

    DeviceStatus status_{DeviceStatus::Unknown};
    bool startRequested_{false};
    bool reconfigureRequested_{false};
    bool disableRequested_{false};
    bool deleteRequested_{false};
    bool faultRequested_{false};
    bool deleted_{false};
    bool runtimeStateDirty_{false};
    DeviceId deviceId_{0};
    DeviceTypeId typeId_{0};
    uint32_t configVersion_{0};
    uint32_t configRevision_{0};
    std::array<DeviceDependencyLink, kMaxDeviceDependencies> dependencyLinks_{};
    uint8_t dependencyCount_{0};
    DevicePersistencePolicy persistencePolicy_{DevicePersistencePolicy::Delayed};

private:
    std::array<IDeviceRuntime*, kMaxDeviceDependencies> dependencyRuntimes_{};
    std::vector<IDeviceRuntime*> dependentRuntimes_{};
};

} // namespace ewfm
