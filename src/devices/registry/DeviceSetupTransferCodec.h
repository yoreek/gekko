#pragma once

#include "devices/registry/DeviceRegistry.h"

#include <ArduinoJson.h>
#include <string>
#include <vector>

namespace ewfm {

class DeviceApiAdapterRegistry;
class IDeviceApiAdapter;

// Serializes the device registry to/from the setup-transfer bundle: NDJSON where every
// device config is human-editable JSON (the same shape the REST API accepts). Per-type
// serialization is delegated to the DeviceApiAdapterRegistry, so every registered type -
// including future ones - round-trips without codec changes.
class DeviceSetupTransferCodec {
public:
    static constexpr uint32_t kTransferSchemaVersion = 3;
    static constexpr uint32_t kMinTransferSchemaVersion = 1;
    // Device configs and setup-only records are separate bounded NDJSON lines. The bundle
    // remains capped independently of the HTTP upload transport.
    static constexpr size_t kMaxBundleBytes =
        (kMaxDynamicDevices * (kMaxDeviceConfigBytes * 2U + 256U)) + (8U * kMaxDisplayLayoutBytes * 3U) + 8192U;

    struct PersistedStateEntry {
        DeviceId deviceId{0};
        std::vector<uint8_t> blob{};
    };

    struct ParseResult {
        DeviceValidationResult validation{};
        DeviceRegistrySnapshot snapshot{};
        DeviceConfigBlobMap configBlobs{};
        uint32_t registryRevision{0};
        size_t deviceCount{0};
        // Owning storage for dynamic per-device error text; validation.message stays a
        // static fallback. Prefer this when non-empty.
        std::string errorDetail{};
        // Display layouts parsed from config.layout, applied after a successful restore.
        std::vector<PersistedStateEntry> persistedStateBlobs{};
        // Serialized dashboard layout object from the dashboard_layout line ("" when absent).
        std::string dashboardLayoutJson{};

        bool ok() const {
            return validation.ok();
        }

        const char* errorMessage() const {
            return errorDetail.empty() ? validation.message : errorDetail.c_str();
        }
    };

    static bool writeBundle(std::string& out, const DeviceRegistry& registry, const DeviceApiAdapterRegistry& adapters,
                            uint32_t registryRevision);
    static ParseResult parseFile(const char* path, size_t fileSize, const DeviceApiAdapterRegistry& adapters);

    static void writeDeviceRecordConfig(JsonObject output, const IDeviceRuntime& runtime, const IDeviceApiAdapter& adapter);

    static std::string encodeHex(const uint8_t* data, size_t size);
    static bool decodeHex(const std::string& hex, std::vector<uint8_t>& bytes);
    static uint32_t checksum(const uint8_t* data, size_t size);
};

} // namespace ewfm
