#pragma once

#include "devices/registry/DeviceRegistry.h"

#if defined(ARDUINO) && !defined(UNIT_TEST)
class AsyncResponseStream;
#endif

#include <string>
#include <vector>

namespace ewfm {

class DeviceSetupTransferCodec {
public:
    static constexpr uint32_t kTransferSchemaVersion = 1;
    static constexpr size_t kMaxBundleBytes = (kMaxDynamicDevices * (kMaxDeviceConfigBytes * 2U + 256U)) + 4096U;

    struct ParseResult {
        DeviceValidationResult validation{};
        DeviceRegistrySnapshot snapshot{};
        DeviceConfigBlobMap configBlobs{};
        uint32_t registryRevision{0};
        size_t deviceCount{0};

        bool ok() const {
            return validation.ok();
        }
    };

#if defined(ARDUINO) && !defined(UNIT_TEST)
    static bool writeBundle(AsyncResponseStream& out, const DeviceRegistry& registry, uint32_t registryRevision);
#endif
    static bool writeBundle(std::string& out, const DeviceRegistry& registry, uint32_t registryRevision);
    static ParseResult parseFile(const char* path, size_t fileSize);

    static std::string encodeHex(const uint8_t* data, size_t size);
    static bool decodeHex(const std::string& hex, std::vector<uint8_t>& bytes);
    static uint32_t checksum(const uint8_t* data, size_t size);

private:
};

} // namespace ewfm
