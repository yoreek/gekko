#pragma once

#include "config/ConfigStore.h"
#include "devices/DeviceTypes.h"

namespace ewfm {

class DeviceRegistryStore {
public:
    explicit DeviceRegistryStore(IConfigStorage& storage);

    bool begin(bool readOnly = false);
    DeviceValidationResult load(DeviceRegistrySnapshot& snapshot, const DeviceTypeRegistry* typeRegistry = nullptr);
    DeviceValidationResult save(const DeviceRegistrySnapshot& snapshot);

private:
    IConfigStorage& storage_;
};

} // namespace ewfm
