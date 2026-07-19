#pragma once

#include "devices/core/DeviceTypes.h"
#include "devices/registry/DeviceRegistry.h"
#include "integrations/common/JsonChunkProducer.h"

#include <ArduinoJson.h>
#include <memory>
#include <string>
#include <vector>

namespace ewfm {

struct DeviceConfigUpdateRequest {
    BoundedBlob<kMaxDeviceConfigBytes> configBlob{};
    uint32_t configVersion{0};
    DeviceBaseConfigV1 baseConfig{};
    bool depsProvided{false};
    std::array<DeviceDependencyLink, kMaxDeviceDependencies> deps{};
    uint8_t depCount{0};
    BoundedBlob<kMaxDisplayLayoutBytes> persistedStateBlob{};
    bool persistedStateProvided{false};

    bool isEnabled() const {
        return baseConfig.enabled != 0U;
    }
};

struct DeviceCreatePersistenceRequest {
    BoundedBlob<kMaxDisplayLayoutBytes> persistedStateBlob{};
    bool persistedStateProvided{false};
};

class IDeviceSetupImportSession {
public:
    virtual ~IDeviceSetupImportSession() = default;
    virtual bool consumeRecord(const JsonObjectConst& input, const char*& error) = 0;
    virtual bool complete() const = 0;
    virtual bool finalize(DeviceCreateRequest& request, DeviceCreatePersistenceRequest& persistedRequest, const char*& error) = 0;
};

class IDeviceApiAdapter {
public:
    IDeviceApiAdapter() = default;
    IDeviceApiAdapter(const IDeviceApiAdapter&) = delete;
    IDeviceApiAdapter& operator=(const IDeviceApiAdapter&) = delete;
    IDeviceApiAdapter(IDeviceApiAdapter&&) = delete;
    IDeviceApiAdapter& operator=(IDeviceApiAdapter&&) = delete;
    virtual ~IDeviceApiAdapter() = default;

    virtual DeviceTypeId typeId() const = 0;
    virtual const char* typeName() const = 0;
    virtual uint32_t currentConfigVersion() const = 0;
    virtual bool parseCreateRequest(const JsonObjectConst& input, DeviceCreateRequest& request, const char*& error) const = 0;
    virtual bool parseCreatePersistedStateRequest(const JsonObjectConst& input, DeviceCreateRequest& request,
                                                  DeviceCreatePersistenceRequest& persistedRequest, const char*& error) const;
    virtual bool parseUpdateConfigRequest(const JsonObjectConst& input, IDeviceRuntime& runtime, DeviceConfigUpdateRequest& request,
                                          const char*& error) const;
    virtual DeviceValidationResult validateCreateRequest(const DeviceCreateRequest& request, const DeviceRegistry& registry) const;
    virtual DeviceValidationResult validateCreateRequest(const DeviceCreateRequest& request,
                                                         const DeviceCreatePersistenceRequest& persistedRequest,
                                                         const DeviceRegistry& registry) const;
    virtual DeviceValidationResult validateUpdateConfigRequest(const IDeviceRuntime& runtime, const DeviceConfigUpdateRequest& request,
                                                               const DeviceRegistry& registry) const;
    virtual DeviceValidationResult validateSetDepsRequest(const IDeviceRuntime& runtime,
                                                          const std::array<DeviceDependencyLink, kMaxDeviceDependencies>& deps,
                                                          uint8_t depCount, const DeviceRegistry& registry) const;
    virtual bool parseSetDepsRequest(const JsonObjectConst& input, std::array<DeviceDependencyLink, kMaxDeviceDependencies>& deps,
                                     uint8_t& depCount, const char*& error) const;
    virtual void writeDeviceJson(const IDeviceRuntime& runtime, DeviceStatus effectiveStatus, JsonObject output) const = 0;
    // Writes only the persisted config fields (no record/runtime sections); used by the
    // device-setup transfer codec to serialize any registered type generically.
    virtual void writeConfigJson(const IDeviceRuntime& runtime, JsonObject config) const = 0;
    void writeSetupTransferConfigJson(const IDeviceRuntime& runtime, JsonObject config) const;
    virtual std::unique_ptr<IJsonChunkProducer> createLayoutJsonProducer(const IDeviceRuntime& runtime, int onlyPageIndex = -1) const;
    virtual std::unique_ptr<IJsonChunkProducer> createSetupExportJsonProducer(const IDeviceRuntime& runtime) const;
    virtual std::unique_ptr<IDeviceSetupImportSession> createSetupImportSession(DeviceId deviceId) const;

    static bool parseDependenciesJson(const JsonObjectConst& input, std::array<DeviceDependencyLink, kMaxDeviceDependencies>& deps,
                                      uint8_t& depCount, const char*& error, bool required = true);
    static void writeFallbackDeviceJson(const IDeviceRuntime& runtime, DeviceStatus effectiveStatus, const char* typeName,
                                        JsonObject output);

protected:
    static void writeCommonDeviceJson(const IDeviceRuntime& runtime, DeviceStatus effectiveStatus, const char* typeName, JsonObject output);
    static void writeDependenciesJson(JsonArray deps, const IDeviceRuntime& runtime);
};

// Serialize the common device `record` scalars into `record` as {id, typeName[, configVersion],
// configRevision}. When `configVersion >= 0` it is emitted between typeName and configRevision - the
// backup bundle needs it for import/migration, the REST envelope omits it. Shared so the record shape
// has one source of truth across the REST device envelope and the device-setup backup bundle.
void writeDeviceRecordJson(JsonObject record, const IDeviceRuntime& runtime, const char* typeName, int64_t configVersion = -1);
class DeviceApiAdapterRegistry {
public:
    bool registerAdapter(const IDeviceApiAdapter& adapter);
    const IDeviceApiAdapter* find(DeviceTypeId typeId) const;
    const IDeviceApiAdapter* findByName(const char* name) const;

    static DeviceApiAdapterRegistry withDefaults();

private:
    std::vector<const IDeviceApiAdapter*> adapters_{};
};

} // namespace ewfm
