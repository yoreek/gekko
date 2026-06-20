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
    void setParentRuntime(IDeviceRuntime* parentRuntime) override;
    IDeviceRuntime* parentRuntime() const override;
    void attachChildRuntime(IDeviceRuntime* childRuntime) override;
    void detachChildRuntime(IDeviceRuntime* childRuntime) override;
    const std::vector<IDeviceRuntime*>& childRuntimes() const override;
    void requestReconfigure() override;
    void requestDisable() override;
    void requestDelete() override;
    DeviceStatus status() const override;
    void bindDeviceIdentity(const DeviceRegistryEntry& record, const DeviceConfigBlob& config) override;
    DeviceId deviceId() const override;
    DeviceTypeId typeId() const override;
    uint32_t configVersion() const override;
    uint32_t configRevision() const override;
    bool hasParent() const override;
    DeviceId parentDeviceId() const override;
    bool enabled() const override;
    const char* name() const override;
    DevicePersistencePolicy persistencePolicy() const override;
    bool handleCommand(const DeviceCommand& command) override;

protected:
    void tickRuntime(uint32_t now);
    void setStatus(DeviceStatus status);
    bool parentReady() const;
    bool hasChildRuntime(const IDeviceRuntime* childRuntime) const;
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
    bool hasParent_{false};
    DeviceId parentDeviceId_{0};
    bool enabled_{true};
    char name_[kMaxDeviceBaseNameLength + 1]{};
    DevicePersistencePolicy persistencePolicy_{DevicePersistencePolicy::Delayed};

private:
    IDeviceRuntime* parentRuntime_{nullptr};
    std::vector<IDeviceRuntime*> childRuntimes_{};
};

} // namespace ewfm
