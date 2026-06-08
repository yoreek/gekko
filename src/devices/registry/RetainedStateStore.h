#pragma once

#include "config/ConfigStore.h"
#include "devices/core/DeviceTypes.h"

namespace ewfm {

class RetainedStateStore {
public:
    explicit RetainedStateStore(IConfigStorage& storage);

    bool begin(bool readOnly = false);
    DeviceValidationResult load(DeviceId deviceId, RetainedStateRecord& record);
    DeviceValidationResult save(const RetainedStateRecord& record);
    bool remove(DeviceId deviceId);

private:
    IConfigStorage& storage_;
};

} // namespace ewfm
