#pragma once

#include "devices/core/DeviceTypes.h"

#include <string>
#include <vector>

namespace ewfm {

class DeviceRegistryBinaryCodec {
public:
    static std::string makeRecordKey(DeviceId deviceId);

    static bool fromHex(const std::string& hex, std::string& bytes);
    static std::string toHex(const std::string& bytes);

    static std::string serializeIndex(const DeviceRegistrySnapshot& snapshot);
    static DeviceValidationResult parseIndex(const std::string& blob, DeviceRegistrySnapshot& snapshot);

    static std::string serializeRecord(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);
    static DeviceValidationResult parseRecord(const std::string& blob, DeviceRegistryEntry& record, DeviceConfigBlob& configBlob);
    static std::vector<uint8_t> serializeRecordBlob(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);
    static DeviceValidationResult parseRecordBlob(const std::vector<uint8_t>& blob, DeviceRegistryEntry& record,
                                                  DeviceConfigBlob& configBlob);
    static size_t serializeRecord(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob, uint8_t* out, size_t capacity);
    static DeviceValidationResult parseRecord(const uint8_t* blob, size_t size, DeviceRegistryEntry& record, DeviceConfigBlob& configBlob);
};

} // namespace ewfm
