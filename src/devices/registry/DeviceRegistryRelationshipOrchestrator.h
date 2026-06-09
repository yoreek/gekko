#pragma once

#include "devices/core/DeviceTypes.h"
#include "devices/registry/DeviceRuntimeStore.h"

namespace ewfm {

class DeviceRegistryRelationshipOrchestrator {
public:
    static void syncRuntimeParentLink(DeviceId deviceId, const DeviceRegistrySnapshot& snapshot, DeviceRuntimeMap& runtimes);
    static void refreshDependentRuntimeStates(const DeviceRegistrySnapshot& snapshot, DeviceRuntimeMap& runtimes);
    static DeviceStatus effectiveStatusForRecord(const DeviceRecord& record, const DeviceRegistrySnapshot& snapshot,
                                                 const DeviceRuntimeMap& runtimes);

private:
    static const DeviceRecord* findRecord(const DeviceRegistrySnapshot& snapshot, DeviceId deviceId);
    static IDeviceRuntime* runtimeFor(DeviceId deviceId, const DeviceRuntimeMap& runtimes);
    static bool parentBlocksChildren(DeviceStatus status);
};

} // namespace ewfm
