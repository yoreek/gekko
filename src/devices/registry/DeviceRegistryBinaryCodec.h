#pragma once

#include "devices/core/DeviceTypes.h"

#include <string>

namespace ewfm {

class DeviceRegistryBinaryCodec {
public:
    static std::string makeRecordKey(DeviceId deviceId);

    static bool fromHex(const std::string& hex, std::string& bytes);
    static std::string toHex(const std::string& bytes);

    static uint32_t payloadChecksum(const std::string& payload);

    static std::string serializeIndex(const DeviceRegistrySnapshot& snapshot);
    static DeviceValidationResult parseIndex(const std::string& blob, DeviceRegistrySnapshot& snapshot);

    static std::string serializeRecord(const DeviceRecord& record);
    static DeviceValidationResult parseRecord(const std::string& blob, DeviceRecord& record);
};

} // namespace ewfm
