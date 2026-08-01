#include "devices/registry/DeviceSetupTransferCodec.h"

#include "integrations/common/DeviceApiAdapter.h"
#include "platform/BoardPinCapabilities.h"

#include <cstring>
#include <memory>
#include <string>
#include <vector>

#if defined(ARDUINO) && !defined(UNIT_TEST)
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#else
#include <ArduinoJson.h>
#include <fstream>
#endif

namespace ewfm {

namespace {
constexpr char kRecordKindEnvelope[] = "transfer_envelope";
constexpr char kRecordKindDevice[] = "device";
constexpr char kRecordKindDashboardLayout[] = "dashboard_layout";
// A v3 line contains at most one display widget, including its base64 bitmap.
constexpr size_t kTransferLineDocBytes = 8192;

template <typename Writer> bool writeLine(Writer& writer, const std::string& line) {
    if (line.empty()) {
        return true;
    }
    return writer.write(reinterpret_cast<const uint8_t*>(line.data()), line.size()) == line.size() &&
           writer.write(reinterpret_cast<const uint8_t*>("\n"), 1U) == 1U;
}

struct StringWriter {
    std::string& out;
    size_t write(const uint8_t* data, size_t size) {
        out.append(reinterpret_cast<const char*>(data), size);
        return size;
    }
};

template <typename Writer> class WriterJsonSink final : public IJsonChunkSink {
public:
    explicit WriterJsonSink(Writer& writer) : writer_(writer) {}

    bool emit(const char* data, const size_t size) override {
        return writer_.write(reinterpret_cast<const uint8_t*>(data), size) == size;
    }

    bool emitJson(JsonDocument& document, const bool leadingComma) override {
        std::string json;
        if (leadingComma) {
            json += ',';
        }
        serializeJson(document, json);
        return emit(json.data(), json.size());
    }

private:
    Writer& writer_;
};

std::string deviceLinePrefix(const uint32_t deviceId, const char* typeName) {
    std::string prefix = "device ";
    prefix += std::to_string(deviceId);
    if (typeName != nullptr && typeName[0] != '\0') {
        prefix += " (";
        prefix += typeName;
        prefix += ")";
    }
    prefix += ": ";
    return prefix;
}

bool parseDeviceLine(const JsonObjectConst& input, const DeviceApiAdapterRegistry& adapters, const bool parseEmbeddedSetupFields,
                     const IDeviceApiAdapter*& adapter, DeviceCreateRequest& request, DeviceRegistryEntry& record,
                     DeviceConfigBlob& configBlob, DeviceCreatePersistenceRequest& persistedRequest, std::string& error) {
    const JsonObjectConst recordInput = input["record"].as<JsonObjectConst>();
    const JsonObjectConst configInput = input["config"].as<JsonObjectConst>();
    if (recordInput.isNull() || configInput.isNull()) {
        error = "device bundle record must contain record and config sections";
        return false;
    }

    const uint32_t deviceId = recordInput["id"] | 0U;
    const char* typeName = recordInput["typeName"] | "";
    if (deviceId == 0U) {
        error = "device bundle record is missing a device id";
        return false;
    }
    if (typeName[0] == '\0') {
        error = deviceLinePrefix(deviceId, nullptr) + "device bundle type is missing";
        return false;
    }

    adapter = adapters.findByName(typeName);
    if (adapter == nullptr) {
        error = deviceLinePrefix(deviceId, nullptr) + "unknown device type '" + typeName + "'";
        return false;
    }

    const uint32_t bundleConfigVersion = recordInput["configVersion"] | 0U;
    const uint32_t currentConfigVersion = adapter->currentConfigVersion();

    request = {};
    const char* parseError = nullptr;
    if (!adapter->parseCreateRequest(input, request, parseError)) {
        error = deviceLinePrefix(deviceId, typeName) + (parseError != nullptr ? parseError : "device config is invalid");
        if (bundleConfigVersion != 0U && bundleConfigVersion < currentConfigVersion) {
            error += " (bundle config version " + std::to_string(bundleConfigVersion) + " predates this firmware's version " +
                     std::to_string(currentConfigVersion) + "; update the config fields to the current format)";
        }
        return false;
    }

    persistedRequest = {};
    if (parseEmbeddedSetupFields) {
        parseError = nullptr;
        if (!adapter->parseCreatePersistedStateRequest(input, request, persistedRequest, parseError)) {
            error = deviceLinePrefix(deviceId, typeName) + (parseError != nullptr ? parseError : "device persisted state is invalid");
            return false;
        }
    }

    record = {};
    record.header.recordVersion = kDeviceRecordHeaderVersion;
    record.header.deviceId = static_cast<DeviceId>(deviceId);
    record.header.typeId = adapter->typeId();
    record.header.configVersion = request.configVersion;
    record.header.configRevision = recordInput["configRevision"] | 1U;
    configBlob = request.configBlob;
    record.header.payloadLength = static_cast<uint32_t>(configBlob.size());
    record.header.payloadChecksum = DeviceSetupTransferCodec::checksum(configBlob.data(), configBlob.size());
    record.persistencePolicy = DevicePersistencePolicy::Delayed;
    record.status = request.baseConfig.enabled != 0U ? DeviceStatus::Ready : DeviceStatus::Disabled;

    if (configInput["deps"].isNull()) {
        // Hand-written bundles may omit deps; the adapter derives them from the config
        // (e.g. bus device ids, layout metric sources) exactly like a REST create.
    } else {
        const char* depsError = nullptr;
        if (!adapter->parseSetDepsRequest(configInput, request.deps, request.depCount, depsError)) {
            error = deviceLinePrefix(deviceId, typeName) + (depsError != nullptr ? depsError : "device bundle deps are invalid");
            return false;
        }
    }

    record.deps = request.deps;
    record.depCount = request.depCount;
    return true;
}

struct PendingDeviceImport {
    const IDeviceApiAdapter* adapter{nullptr};
    DeviceCreateRequest request{};
    DeviceRegistryEntry record{};
    DeviceConfigBlob configBlob{};
    DeviceCreatePersistenceRequest persistedRequest{};
    std::unique_ptr<IDeviceSetupImportSession> setupSession{};
};

template <typename Writer>
bool writeBundleImpl(Writer& writer, const DeviceRegistry& registry, const DeviceApiAdapterRegistry& adapters,
                     const uint32_t registryRevision, const char* boardId) {
    StaticJsonDocument<256> envelope;
    envelope["kind"] = kRecordKindEnvelope;
    envelope["transferSchemaVersion"] = DeviceSetupTransferCodec::kTransferSchemaVersion;
    envelope["registrySchemaVersion"] = kDeviceRegistrySchemaVersion;
    envelope["registryRevision"] = registryRevision;
    envelope["chip"] = kChipId;
    envelope["boardId"] = boardId;

    size_t deviceCount = 0;
    registry.forEachRuntime([&](const IDeviceRuntime& runtime) {
        (void)runtime;
        ++deviceCount;
    });
    envelope["deviceCount"] = deviceCount;

    std::string line;
    line.reserve(kTransferLineDocBytes);
    serializeJson(envelope, line);
    if (!writeLine(writer, line)) {
        return false;
    }

    auto doc = std::make_unique<DynamicJsonDocument>(kTransferLineDocBytes);
    if (!doc || doc->capacity() == 0U) {
        return false;
    }

    bool ok = true;
    registry.forEachRuntime([&](const IDeviceRuntime& runtime) {
        if (!ok) {
            return;
        }
        // A registered runtime without a REST adapter would silently fall out of backups;
        // fail the export instead so the gap is caught immediately.
        const IDeviceApiAdapter* adapter = adapters.find(runtime.typeId());
        if (adapter == nullptr) {
            ok = false;
            return;
        }

        doc->clear();
        JsonObject output = doc->to<JsonObject>();
        DeviceSetupTransferCodec::writeDeviceRecordConfig(output, runtime, *adapter);

        if (doc->overflowed()) {
            ok = false;
            return;
        }
        line.clear();
        serializeJson(*doc, line);
        if (!writeLine(writer, line)) {
            ok = false;
            return;
        }

        std::unique_ptr<IJsonChunkProducer> setupProducer = adapter->createSetupExportJsonProducer(runtime);
        if (setupProducer != nullptr) {
            WriterJsonSink<Writer> sink(writer);
            while (setupProducer->next(sink)) {
            }
        }
    });
    return ok;
}

} // namespace

std::string DeviceSetupTransferCodec::encodeHex(const uint8_t* data, const size_t size) {
    static constexpr char kHexDigits[] = "0123456789abcdef";
    std::string hex;
    hex.reserve(size * 2U);
    for (size_t index = 0; index < size; ++index) {
        const uint8_t byte = data[index];
        hex.push_back(kHexDigits[(byte >> 4U) & 0x0FU]);
        hex.push_back(kHexDigits[byte & 0x0FU]);
    }
    return hex;
}

bool DeviceSetupTransferCodec::decodeHex(const std::string& hex, std::vector<uint8_t>& bytes) {
    auto hexDigit = [](char ch) -> int {
        if (ch >= '0' && ch <= '9') {
            return ch - '0';
        }
        if (ch >= 'a' && ch <= 'f') {
            return 10 + (ch - 'a');
        }
        if (ch >= 'A' && ch <= 'F') {
            return 10 + (ch - 'A');
        }
        return -1;
    };

    if ((hex.size() % 2U) != 0U) {
        return false;
    }
    bytes.clear();
    bytes.reserve(hex.size() / 2U);
    for (size_t index = 0; index < hex.size(); index += 2U) {
        const int hi = hexDigit(hex[index]);
        const int lo = hexDigit(hex[index + 1U]);
        if (hi < 0 || lo < 0) {
            return false;
        }
        bytes.push_back(static_cast<uint8_t>((hi << 4U) | lo));
    }
    return true;
}

uint32_t DeviceSetupTransferCodec::checksum(const uint8_t* data, const size_t size) {
    uint32_t hash = 2166136261UL;
    for (size_t index = 0; index < size; ++index) {
        hash ^= data[index];
        hash *= 16777619UL;
    }
    return hash;
}

void DeviceSetupTransferCodec::writeDeviceRecordConfig(JsonObject output, const IDeviceRuntime& runtime, const IDeviceApiAdapter& adapter) {
    output["kind"] = kRecordKindDevice;
    JsonObject record = output.createNestedObject("record");
    // Unlike the REST envelope, the bundle records configVersion so import can migrate old configs.
    writeDeviceRecordJson(record, runtime, adapter.typeName(), static_cast<int64_t>(adapter.currentConfigVersion()));

    JsonObject config = output.createNestedObject("config");
    adapter.writeSetupTransferConfigJson(runtime, config);
}

bool DeviceSetupTransferCodec::writeBundle(std::string& out, const DeviceRegistry& registry, const DeviceApiAdapterRegistry& adapters,
                                           uint32_t registryRevision, const char* boardId) {
    StringWriter writer{out};
    out.clear();
    return writeBundleImpl(writer, registry, adapters, registryRevision, boardId);
}

DeviceSetupTransferCodec::ParseResult DeviceSetupTransferCodec::parseFile(const char* path, const size_t fileSize,
                                                                          const DeviceApiAdapterRegistry& adapters) {
    ParseResult result{};
    if (path == nullptr || path[0] == '\0') {
        result.validation = {DeviceError::InvalidConfig, "bundle file path is missing"};
        return result;
    }
    if (fileSize == 0U || fileSize > kMaxBundleBytes) {
        result.validation = {DeviceError::BoundsExceeded, "bundle file exceeds supported size"};
        return result;
    }

    std::string line;
    bool envelopeSeen = false;
    bool dashboardSeen = false;
    uint32_t transferSchemaVersion = 0U;
    bool deviceCountProvided = false;
    size_t expectedDeviceCount = 0;
    DeviceRegistrySnapshot snapshot{};
    DeviceConfigBlobMap configBlobs{};
    std::unique_ptr<PendingDeviceImport> pending;

    auto doc = std::make_unique<DynamicJsonDocument>(kTransferLineDocBytes);
    if (!doc || doc->capacity() == 0U) {
        result.validation = {DeviceError::StorageError, "bundle parser allocation failed"};
        return result;
    }

    const auto appendPending = [&]() -> bool {
        if (pending == nullptr) {
            return true;
        }
        if (pending->setupSession != nullptr) {
            if (!pending->setupSession->complete()) {
                result.errorDetail =
                    deviceLinePrefix(pending->record.header.deviceId, pending->adapter->typeName()) + "setup records are incomplete";
                result.validation = {DeviceError::InvalidConfig, "device bundle entry is incomplete"};
                return false;
            }
            const char* finalizeError = nullptr;
            if (!pending->setupSession->finalize(pending->request, pending->persistedRequest, finalizeError)) {
                result.errorDetail = deviceLinePrefix(pending->record.header.deviceId, pending->adapter->typeName()) +
                                     (finalizeError != nullptr ? finalizeError : "device setup records are invalid");
                result.validation = {DeviceError::InvalidConfig, "device bundle entry is invalid"};
                return false;
            }
            pending->record.deps = pending->request.deps;
            pending->record.depCount = pending->request.depCount;
        }

        snapshot.records.push_back(pending->record);
        snapshot.indexEntries.push_back({pending->record.header.deviceId, pending->record.header.typeId});
        configBlobs[pending->record.header.deviceId] = pending->configBlob;
        if (pending->persistedRequest.persistedStateProvided) {
            PersistedStateEntry entry{};
            entry.deviceId = pending->record.header.deviceId;
            entry.blob.assign(pending->persistedRequest.persistedStateBlob.data(),
                              pending->persistedRequest.persistedStateBlob.data() + pending->persistedRequest.persistedStateBlob.size());
            result.persistedStateBlobs.push_back(std::move(entry));
        }
        pending.reset();
        return true;
    };

#if defined(ARDUINO) && !defined(UNIT_TEST)
    File file = LittleFS.open(path, "r");
    if (!file) {
        result.validation = {DeviceError::MissingRecord, "bundle file is missing"};
        return result;
    }
    while (file.available()) {
        line.clear();
        while (file.available()) {
            const int value = file.read();
            if (value < 0 || value == '\n') {
                break;
            }
            if (line.size() >= kTransferLineDocBytes) {
                result.validation = {DeviceError::BoundsExceeded, "bundle line exceeds supported size"};
                return result;
            }
            line.push_back(static_cast<char>(value));
        }
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
#else
    std::ifstream file(path);
    if (!file.is_open()) {
        result.validation = {DeviceError::MissingRecord, "bundle file is missing"};
        return result;
    }
    while (std::getline(file, line)) {
#endif
        if (line.empty()) {
            continue;
        }
        if (line.size() > kTransferLineDocBytes) {
            result.validation = {DeviceError::BoundsExceeded, "bundle line exceeds supported size"};
            return result;
        }
        doc->clear();
        const DeserializationError error = deserializeJson(*doc, line);
        if (error) {
            result.validation = {DeviceError::CorruptRecord, "bundle line is too large or not valid JSON"};
            return result;
        }
        const JsonObjectConst object = doc->as<JsonObjectConst>();
        const char* kind = object["kind"] | "";
        if (!envelopeSeen) {
            if (std::strcmp(kind, kRecordKindEnvelope) != 0) {
                result.validation = {DeviceError::InvalidConfig, "bundle is missing the transfer envelope"};
                return result;
            }
            transferSchemaVersion = object["transferSchemaVersion"] | 0U;
            if (transferSchemaVersion < kMinTransferSchemaVersion || transferSchemaVersion > kTransferSchemaVersion) {
                result.validation = {DeviceError::InvalidVersion, "unsupported transfer schema version"};
                return result;
            }
            result.registryRevision = object["registryRevision"] | 0U;
            result.chip = std::string(object["chip"] | "");
            result.boardId = std::string(object["boardId"] | "");
            deviceCountProvided = !object["deviceCount"].isNull();
            expectedDeviceCount = object["deviceCount"] | 0U;
            envelopeSeen = true;
            continue;
        }

        if (std::strcmp(kind, kRecordKindDashboardLayout) == 0) {
            if (!appendPending()) {
                return result;
            }
            if (dashboardSeen) {
                result.validation = {DeviceError::InvalidConfig, "bundle contains more than one dashboard layout"};
                return result;
            }
            const JsonVariantConst layout = object["layout"];
            if (layout.isNull()) {
                result.validation = {DeviceError::InvalidConfig, "dashboard layout line is missing the layout object"};
                return result;
            }
            serializeJson(layout, result.dashboardLayoutJson);
            dashboardSeen = true;
            continue;
        }

        if (std::strcmp(kind, kRecordKindDevice) == 0) {
            if (dashboardSeen && transferSchemaVersion >= 3U) {
                result.validation = {DeviceError::InvalidConfig, "device records must precede dashboard layout"};
                return result;
            }
            if (!appendPending()) {
                return result;
            }

            pending = std::make_unique<PendingDeviceImport>();
            if (!pending) {
                result.validation = {DeviceError::StorageError, "bundle parser allocation failed"};
                return result;
            }
            std::string errorMessage;
            if (!parseDeviceLine(object, adapters, transferSchemaVersion < 3U, pending->adapter, pending->request, pending->record,
                                 pending->configBlob, pending->persistedRequest, errorMessage)) {
                result.errorDetail = std::move(errorMessage);
                result.validation = {DeviceError::InvalidConfig, "device bundle entry is invalid"};
                return result;
            }
            if (transferSchemaVersion >= 3U) {
                pending->setupSession = pending->adapter->createSetupImportSession(pending->record.header.deviceId);
            }
            if (pending->setupSession == nullptr && !appendPending()) {
                return result;
            }
            continue;
        }

        if (transferSchemaVersion < 3U || pending == nullptr || pending->setupSession == nullptr) {
            result.validation = {DeviceError::InvalidConfig, "unexpected bundle record kind"};
            return result;
        }
        const char* setupError = nullptr;
        if (!pending->setupSession->consumeRecord(object, setupError)) {
            result.errorDetail = deviceLinePrefix(pending->record.header.deviceId, pending->adapter->typeName()) +
                                 (setupError != nullptr ? setupError : "device setup record is invalid");
            result.validation = {DeviceError::InvalidConfig, "device bundle entry is invalid"};
            return result;
        }
        if (pending->setupSession->complete() && !appendPending()) {
            return result;
        }
    }

    if (!envelopeSeen) {
        result.validation = {DeviceError::InvalidConfig, "bundle is missing the transfer envelope"};
        return result;
    }
    if (!appendPending()) {
        return result;
    }

    if (deviceCountProvided && expectedDeviceCount != snapshot.records.size()) {
        result.validation = {DeviceError::InvalidConfig, "bundle device count mismatch"};
        return result;
    }

    result.deviceCount = snapshot.records.size();
    result.snapshot = std::move(snapshot);
    result.configBlobs = std::move(configBlobs);
    return result;
}

} // namespace ewfm
