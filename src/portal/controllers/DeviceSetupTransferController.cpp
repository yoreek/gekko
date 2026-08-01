#include "portal/controllers/DeviceSetupTransferController.h"

#include "config/ConfigStore.h"
#include "devices/registry/DeviceSetupTransferCodec.h"
#include "integrations/common/DeviceApiAdapter.h"
#include "platform/BoardPinCapabilities.h"
#include "portal/DashboardLayoutStore.h"

#include <memory>
#include <string>
#include <vector>

#if defined(ARDUINO) && !defined(UNIT_TEST)
#include "debug/Debug.h"

#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <cstdio>
#endif

namespace ewfm {

namespace {
#if defined(ARDUINO) && !defined(UNIT_TEST)
const char* errorCodeForTransferError(const DeviceError error) {
    switch (error) {
    case DeviceError::UnsupportedType:
        return "UNSUPPORTED_TYPE";
    case DeviceError::InvalidDeviceId:
        return "INVALID_DEVICE_ID";
    case DeviceError::DuplicateDeviceId:
        return "DUPLICATE_DEVICE_ID";
    case DeviceError::InvalidRelationship:
        return "INVALID_RELATIONSHIP";
    case DeviceError::BoundsExceeded:
        return "BOUNDS_EXCEEDED";
    case DeviceError::StorageError:
        return "STORAGE_ERROR";
    case DeviceError::InvalidVersion:
        return "INVALID_VERSION";
    case DeviceError::CorruptRecord:
        return "BAD_JSON";
    case DeviceError::MissingRecord:
        return "NOT_FOUND";
    case DeviceError::InvalidCommand:
        return "INVALID_COMMAND";
    case DeviceError::InvalidConfig:
    case DeviceError::None:
    default:
        return "BAD_ARGS";
    }
}

// A device's record + config WITHOUT the layout - well under 2 KB now that the layout streams
// separately (same rationale as kDeviceJsonCapacity in DeviceRegistryController).
constexpr size_t kExportDeviceJsonCapacity = 2048;
constexpr size_t kExportDashboardJsonCapacity = DashboardLayoutStore::kMaxSerializedBytes + 512U;

// Producer state for GET /api/device-setup/export, held alive by the chunked-response filler (via
// shared_ptr) because the filler runs on the async_tcp task after show() has already returned. Peak
// contiguous memory is bounded to one display widget (a base64 bitmap), never a whole device line or
// the whole bundle: each device's record/config streams as one small line, and a display device's
// layout streams one widget per chunk - the same discipline as GET /api/devices/:id/layout. All the
// reused serialization docs live inline in this one heap-held state, so there is zero per-line heap
// churn and nothing large on the async_tcp stack.
struct ExportProducerState {
    DeviceRegistry* registry = nullptr;
    const DeviceApiAdapterRegistry* adapters = nullptr;
    DashboardLayoutStore* dashboardLayoutStore = nullptr;
    ConfigStore* configStore = nullptr;
    uint32_t registryRevision = 0;

    std::vector<DeviceId> deviceIds; // snapshot taken up front
    size_t cursor = 0;
    int phase = 0; // 0 = envelope, 1 = devices, 2 = dashboard layout, 3 = done

    // Per-device sub-state machine: one complete device line followed by optional
    // adapter-owned setup lines.
    int deviceStep = 0;
    std::unique_ptr<IJsonChunkProducer> deviceExtensionProducer;

    std::string scratch; // reused per line, so capacity stabilizes after warmup
    StaticJsonDocument<kExportDeviceJsonCapacity> metaDoc;
    StaticJsonDocument<kExportDashboardJsonCapacity> dashboardDoc;
};

class ChunkedBodyJsonSink final : public IJsonChunkSink {
public:
    explicit ChunkedBodyJsonSink(BaseController::ChunkedBody& body) : body_(body) {}

    bool emit(const char* data, const size_t size) override {
        return body_.emit(data, size);
    }

    bool emitJson(JsonDocument& document, const bool leadingComma) override {
        return body_.emitJson(document, leadingComma);
    }

private:
    BaseController::ChunkedBody& body_;
};

// Produce the next NDJSON piece into `body`. Returns false only when the whole bundle is done.
bool produceDeviceSetupExportPiece(ExportProducerState& s, BaseController::ChunkedBody& body) {
    if (s.phase == 0) {
        StaticJsonDocument<256> envelope;
        envelope["kind"] = "transfer_envelope";
        envelope["transferSchemaVersion"] = DeviceSetupTransferCodec::kTransferSchemaVersion;
        envelope["registrySchemaVersion"] = kDeviceRegistrySchemaVersion;
        envelope["registryRevision"] = s.registryRevision;
        envelope["deviceCount"] = s.deviceIds.size();
        envelope["chip"] = kChipId;
        envelope["boardId"] = (s.configStore != nullptr) ? boardModelName(s.configStore->config().boardModel) : "";
        s.scratch.clear();
        serializeJson(envelope, s.scratch);
        s.scratch += "\n";
        s.phase = 1;
        return body.emit(s.scratch.data(), s.scratch.size());
    }

    if (s.phase == 1) {
        while (s.cursor < s.deviceIds.size()) {
            // The runtime is only consulted on deviceStep 0. The adapter producer owns a snapshot,
            // so deleting the device mid-stream cannot abandon a half-written line.
            if (s.deviceStep == 0) {
                const DeviceId id = s.deviceIds[s.cursor];
                const IDeviceRuntime* runtime = s.registry->runtime(id);
                const IDeviceApiAdapter* adapter = (runtime != nullptr) ? s.adapters->find(runtime->typeId()) : nullptr;
                if (runtime == nullptr || adapter == nullptr) {
                    // Vanished between snapshot and now, or no REST adapter - omit it and move on.
                    ++s.cursor;
                    continue;
                }
                s.metaDoc.clear();
                JsonObject output = s.metaDoc.to<JsonObject>();
                DeviceSetupTransferCodec::writeDeviceRecordConfig(output, *runtime, *adapter);
                if (s.metaDoc.overflowed()) {
                    EWFM_LOG_WARN("json", "device-setup export skip id=%lu (record/config exceeds %u-byte doc)",
                                  static_cast<unsigned long>(id), static_cast<unsigned>(kExportDeviceJsonCapacity));
                    ++s.cursor;
                    s.deviceStep = 0;
                    continue;
                }
                s.scratch.clear();
                serializeJson(s.metaDoc, s.scratch);
                s.scratch += "\n";
                s.deviceExtensionProducer = adapter->createSetupExportJsonProducer(*runtime);
                s.deviceStep = 1;
                return body.emit(s.scratch.data(), s.scratch.size());
            }

            if (s.deviceStep == 1) {
                if (s.deviceExtensionProducer != nullptr) {
                    ChunkedBodyJsonSink sink(body);
                    if (s.deviceExtensionProducer->next(sink)) {
                        return true;
                    }
                    s.deviceExtensionProducer.reset();
                }
                ++s.cursor;
                s.deviceStep = 0;
            }
        }
        s.phase = 2;
    }

    if (s.phase == 2) {
        s.phase = 3;
        if (s.dashboardLayoutStore != nullptr) {
            const DashboardLayoutLoadResult loaded = s.dashboardLayoutStore->load();
            if (loaded.validation.ok() && !loaded.defaulted) {
                s.dashboardDoc.clear();
                s.dashboardDoc["kind"] = "dashboard_layout";
                s.dashboardDoc["revision"] = loaded.revision;
                JsonObject layout = s.dashboardDoc.createNestedObject("layout");
                s.dashboardLayoutStore->writeLayoutJson(layout, loaded.layout);
                if (!s.dashboardDoc.overflowed()) {
                    s.scratch.clear();
                    serializeJson(s.dashboardDoc, s.scratch);
                    s.scratch += "\n";
                    return body.emit(s.scratch.data(), s.scratch.size());
                }
            }
        }
    }

    return false;
}
#endif
} // namespace

DeviceSetupTransferController::DeviceSetupTransferController(AsyncWebServerRequest* request, const Action action, DeviceRegistry& registry,
                                                             const DeviceApiAdapterRegistry& adapters,
                                                             DashboardLayoutStore* dashboardLayoutStore, ConfigStore* configStore)
    : BaseController(request, action), registry_(registry), adapters_(adapters), dashboardLayoutStore_(dashboardLayoutStore),
      configStore_(configStore) {}

#if defined(ARDUINO) && !defined(UNIT_TEST)
void DeviceSetupTransferController::registerRoutes(AsyncWebServer& server, DeviceRegistry& registry,
                                                   DashboardLayoutStore* dashboardLayoutStore, ConfigStore* configStore) {
    static const DeviceApiAdapterRegistry adapters = DeviceApiAdapterRegistry::withDefaults();
    server.on("/api/device-setup/export", HTTP_GET, [&registry, dashboardLayoutStore, configStore](AsyncWebServerRequest* request) {
        DeviceSetupTransferController(request, Action::Show, registry, adapters, dashboardLayoutStore, configStore).dispatch();
    });
    server.on(
        "/api/device-setup/import", HTTP_POST,
        [&registry, dashboardLayoutStore, configStore](AsyncWebServerRequest* request) {
            DeviceSetupTransferController(request, Action::Create, registry, adapters, dashboardLayoutStore, configStore).dispatch();
        },
        [](AsyncWebServerRequest* request, const String& filename, size_t index, uint8_t* data, size_t len, bool final) {
            size_t fileIndex = 0U;
            if (index == 0U) {
                if (!BaseController::beginUploadFile(request, filename.c_str(), "application/x-ndjson", 0U, fileIndex)) {
                    return;
                }
            }

            const size_t total = index + len;
            if (!BaseController::appendUploadFile(request, fileIndex, data, len, index, total)) {
                return;
            }
            if (final) {
                (void)BaseController::finishUploadFile(request, fileIndex, true);
            }
        },
        nullptr);
    server.on("/api/device-setup/export", HTTP_OPTIONS, [&registry, dashboardLayoutStore, configStore](AsyncWebServerRequest* request) {
        DeviceSetupTransferController(request, Action::Options, registry, adapters, dashboardLayoutStore, configStore).dispatch();
    });
    server.on("/api/device-setup/import", HTTP_OPTIONS, [&registry, dashboardLayoutStore, configStore](AsyncWebServerRequest* request) {
        DeviceSetupTransferController(request, Action::Options, registry, adapters, dashboardLayoutStore, configStore).dispatch();
    });
}
#endif

const BaseController::RulesChain* DeviceSetupTransferController::beforeChain() {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    static constexpr HookRule rules[] = {
        {&BaseController::beforeCorsOptions, ALL},
    };
    static const RulesChain node{rules, sizeof(rules) / sizeof(rules[0]), nullptr};
    return &node;
#else
    return BaseController::beforeChain();
#endif
}

void DeviceSetupTransferController::show() {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    // Stream the NDJSON bundle piece-by-piece instead of buffering it whole in an AsyncResponseStream:
    // peak contiguous allocation stays bounded to one display widget regardless of device count. See
    // ExportProducerState above.
    auto state = std::make_shared<ExportProducerState>();
    state->registry = &registry_;
    state->adapters = &adapters_;
    state->dashboardLayoutStore = dashboardLayoutStore_;
    state->configStore = configStore_;
    state->registryRevision = registry_.registryRevision();
    registry_.forEachRuntime([&state](const IDeviceRuntime& runtime) { state->deviceIds.push_back(runtime.deviceId()); });

    sendChunked(
        "application/x-ndjson", [state](ChunkedBody& body) { return produceDeviceSetupExportPiece(*state, body); },
        [](AsyncWebServerResponse* response) {
            if (response != nullptr) {
                response->addHeader("Content-Disposition", "attachment; filename=device-setup.ndjson");
            }
        });
#endif
}

void DeviceSetupTransferController::create() {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    size_t fileCount = 0U;
    const RequestFile* files = requestFiles(request_, fileCount);
    if (files == nullptr || fileCount == 0U) {
        renderError(400, "BAD_ARGS", "bundle file is required");
        return;
    }

    const RequestFile& file = files[0];
    if (!file.present || file.tmpPath[0] == '\0') {
        renderError(400, "BAD_ARGS", "bundle file is required");
        return;
    }
    if (file.size > DeviceSetupTransferCodec::kMaxBundleBytes) {
        renderError(413, "BOUNDS_EXCEEDED", "bundle file exceeds supported size");
        return;
    }

    const DeviceSetupTransferCodec::ParseResult parsed = DeviceSetupTransferCodec::parseFile(file.tmpPath, file.size, adapters_);
    if (!parsed.ok()) {
        renderError(parsed.validation.error == DeviceError::BoundsExceeded ? 413 : 400, errorCodeForTransferError(parsed.validation.error),
                    parsed.errorMessage());
        return;
    }

    std::vector<DevicePersistedStateUpdate> persistedStateUpdates;
    persistedStateUpdates.reserve(parsed.persistedStateBlobs.size());
    for (const auto& entry : parsed.persistedStateBlobs) {
        persistedStateUpdates.push_back({entry.deviceId, entry.blob.data(), entry.blob.size()});
    }
    const DeviceValidationResult restoreResult =
        registry_.restore(parsed.snapshot, parsed.configBlobs, persistedStateUpdates, parsed.registryRevision, 0);
    if (!restoreResult.ok()) {
        renderError(restoreResult.error == DeviceError::BoundsExceeded ? 413 : 500, errorCodeForTransferError(restoreResult.error),
                    restoreResult.message);
        return;
    }

    auto doc = std::make_unique<DynamicJsonDocument>(2048U);
    if (!doc || doc->capacity() == 0U) {
        renderError(500, "INTERNAL", "doc allocation failed");
        return;
    }
    JsonArray warnings = doc->createNestedArray("warnings");

    if (!parsed.dashboardLayoutJson.empty()) {
        if (dashboardLayoutStore_ == nullptr) {
            warnings.add("dashboard layout skipped: store unavailable");
        } else {
            auto layoutDoc = std::make_unique<DynamicJsonDocument>(DashboardLayoutStore::kMaxSerializedBytes + 512U);
            if (!layoutDoc || layoutDoc->capacity() == 0U ||
                deserializeJson(*layoutDoc, parsed.dashboardLayoutJson) != DeserializationError::Ok) {
                warnings.add("dashboard layout skipped: layout is not valid JSON");
            } else {
                const DashboardLayoutSaveResult saveResult = dashboardLayoutStore_->saveJson(layoutDoc->as<JsonVariantConst>());
                if (!saveResult.ok()) {
                    std::string warning = "dashboard layout skipped: ";
                    warning += saveResult.validation.message;
                    warnings.add(warning);
                }
            }
        }
    }

    if (!parsed.boardId.empty()) {
        if (configStore_ == nullptr) {
            warnings.add("board model skipped: config store unavailable");
        } else {
            BoardModel parsedBoardModel{};
            if (!boardModelFromString(parsed.boardId.c_str(), parsedBoardModel)) {
                warnings.add("board model skipped: unsupported board id for this chip");
            } else {
                DeviceConfig next = configStore_->config();
                if (next.boardModel != parsedBoardModel) {
                    next.boardModel = parsedBoardModel;
                    const ValidationResult boardSaveResult = configStore_->save(next);
                    if (!boardSaveResult.ok()) {
                        std::string warning = "board model skipped: ";
                        warning += boardSaveResult.message;
                        warnings.add(warning);
                    }
                }
            }
        }
    }

    (*doc)["deviceCount"] = parsed.deviceCount;
    (*doc)["registryRevision"] = registry_.registryRevision();
    renderOk(*doc);
#endif
}

void DeviceSetupTransferController::options() {
    BaseController::options();
}

} // namespace ewfm
