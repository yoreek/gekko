#pragma once

#include "config/ConfigStore.h"
#include "devices/core/DeviceTypes.h"

#include <map>

namespace ewfm {

using DeviceConfigBlobMap = std::map<DeviceId, DeviceConfigBlob>;

class DeviceRegistryStore {
public:
    explicit DeviceRegistryStore(IConfigStorage& storage);

    bool begin(bool readOnly = false);
    DeviceValidationResult load(DeviceRegistrySnapshot& snapshot, DeviceConfigBlobMap& configBlobs,
                                const DeviceTypeRegistry* typeRegistry = nullptr, std::vector<DeviceId>* discardedDeviceIds = nullptr);
    DeviceValidationResult save(const DeviceRegistrySnapshot& snapshot, const DeviceConfigBlobMap& configBlobs);
    DeviceValidationResult saveRecovered(const DeviceRegistrySnapshot& snapshot, const DeviceConfigBlobMap& configBlobs);
    DeviceValidationResult saveRecords(const DeviceRegistrySnapshot& snapshot, const DeviceConfigBlobMap& configBlobs,
                                       const std::vector<DeviceId>& dirtyRecordIds);
    DeviceValidationResult saveIndex(const DeviceRegistrySnapshot& snapshot);
    bool removeRecord(DeviceId deviceId);

private:
    IConfigStorage& storage_;
};

} // namespace ewfm
