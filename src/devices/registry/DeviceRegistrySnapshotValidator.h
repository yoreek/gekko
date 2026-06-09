#pragma once

#include "devices/core/DeviceTypes.h"

namespace ewfm {

class DeviceRegistrySnapshotValidator {
public:
    static DeviceValidationResult validateStructure(const DeviceRegistrySnapshot& snapshot);
    static DeviceValidationResult validateTypedRelationships(const DeviceRegistrySnapshot& snapshot,
                                                             const DeviceTypeRegistry* typeRegistry);
};

} // namespace ewfm
