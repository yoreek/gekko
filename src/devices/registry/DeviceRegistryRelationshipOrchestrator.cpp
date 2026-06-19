#include "devices/registry/DeviceRegistryRelationshipOrchestrator.h"

namespace ewfm {

const DeviceRecord* DeviceRegistryRelationshipOrchestrator::findRecord(const DeviceRegistrySnapshot& snapshot, DeviceId deviceId) {
    for (const auto& record : snapshot.records) {
        if (record.header.deviceId == deviceId) {
            return &record;
        }
    }
    return nullptr;
}

IDeviceRuntime* DeviceRegistryRelationshipOrchestrator::runtimeFor(DeviceId deviceId, const DeviceRuntimeMap& runtimes) {
    const auto it = runtimes.find(deviceId);
    if (it == runtimes.end() || it->second.runtime == nullptr) {
        return nullptr;
    }
    return it->second.runtime.get();
}

bool DeviceRegistryRelationshipOrchestrator::parentBlocksChildren(DeviceStatus status) {
    return status != DeviceStatus::Ready;
}

void DeviceRegistryRelationshipOrchestrator::syncRuntimeParentLink(DeviceId deviceId, const DeviceRegistrySnapshot& snapshot,
                                                                   DeviceRuntimeMap& runtimes) {
    const DeviceRecord* record = findRecord(snapshot, deviceId);
    if (record == nullptr) {
        return;
    }

    const auto runtimeIt = runtimes.find(deviceId);
    if (runtimeIt == runtimes.end() || runtimeIt->second.runtime == nullptr) {
        return;
    }

    IDeviceRuntime* nextParent = nullptr;
    if (record->hasParent) {
        nextParent = runtimeFor(record->parentDeviceId, runtimes);
    }

    IDeviceRuntime* currentParent = runtimeIt->second.runtime->parentRuntime();
    if (currentParent == nextParent) {
        return;
    }

    if (currentParent != nullptr) {
        currentParent->detachChildRuntime(runtimeIt->second.runtime.get());
    }
    runtimeIt->second.runtime->setParentRuntime(nextParent);
    if (nextParent != nullptr) {
        nextParent->attachChildRuntime(runtimeIt->second.runtime.get());
    }
}

DeviceStatus DeviceRegistryRelationshipOrchestrator::effectiveStatusForRecord(const DeviceRecord& record,
                                                                              const DeviceRegistrySnapshot& snapshot,
                                                                              const DeviceRuntimeMap& runtimes) {
    DeviceStatus rawStatus = record.status;
    if (IDeviceRuntime* runtime = runtimeFor(record.header.deviceId, runtimes); runtime != nullptr) {
        rawStatus = runtime->status();
    }

    if (!record.enabled) {
        return DeviceStatus::Disabled;
    }
    if (rawStatus == DeviceStatus::Faulted || rawStatus == DeviceStatus::Deleting) {
        return rawStatus;
    }
    if (!record.hasParent) {
        return rawStatus;
    }

    const DeviceRecord* parent = findRecord(snapshot, record.parentDeviceId);
    if (parent == nullptr) {
        return DeviceStatus::DependencyBlocked;
    }

    const DeviceStatus parentStatus = effectiveStatusForRecord(*parent, snapshot, runtimes);
    if (parentStatus == DeviceStatus::Disabled) {
        return DeviceStatus::Disabled;
    }
    if (parentBlocksChildren(parentStatus)) {
        return DeviceStatus::DependencyBlocked;
    }
    return rawStatus;
}

void DeviceRegistryRelationshipOrchestrator::refreshDependentRuntimeStates(const DeviceRegistrySnapshot& snapshot,
                                                                           DeviceRuntimeMap& runtimes) {
    for (const auto& record : snapshot.records) {
        syncRuntimeParentLink(record.header.deviceId, snapshot, runtimes);

        const auto runtimeIt = runtimes.find(record.header.deviceId);
        if (runtimeIt == runtimes.end() || runtimeIt->second.runtime == nullptr) {
            continue;
        }

        if (!record.enabled) {
            runtimeIt->second.runtime->requestDisable();
            continue;
        }

        if (record.hasParent) {
            const DeviceRecord* parent = findRecord(snapshot, record.parentDeviceId);
            const DeviceStatus parentStatus =
                parent != nullptr ? effectiveStatusForRecord(*parent, snapshot, runtimes) : DeviceStatus::DependencyBlocked;
            if (parentStatus == DeviceStatus::Disabled) {
                runtimeIt->second.runtime->requestDisable();
                continue;
            }
            if (parentBlocksChildren(parentStatus)) {
                continue;
            }
        }

        const DeviceStatus runtimeStatus = runtimeIt->second.runtime->status();
        if (runtimeStatus == DeviceStatus::DependencyBlocked || runtimeStatus == DeviceStatus::Disabled) {
            runtimeIt->second.runtime->requestReconfigure();
        }
    }
}

} // namespace ewfm
