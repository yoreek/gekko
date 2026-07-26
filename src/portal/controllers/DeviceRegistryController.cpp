#include "portal/controllers/DeviceRegistryController.h"

#include "debug/Debug.h"
#include "devices/core/DeviceBaseConfig.h"
#include "devices/dosing/DosingPumpDeviceConfig.h"
#include "integrations/mqtt/HaDeviceSettings.h"

#if defined(WITH_HOME_ASSISTANT)
#include "integrations/mqtt/HaDiscoveryBridge.h"
#endif

#include "platform/ArduinoClock.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

#if defined(ARDUINO) && !defined(UNIT_TEST)
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#endif

namespace ewfm {

class HaEntityAdapterRegistry;

DeviceRegistryController::DeviceRegistryController(AsyncWebServerRequest* request, const Action action, DeviceRegistry& registry,
                                                   const DeviceApiAdapterRegistry& adapters, DeviceScopedDataStore* haSettingsStore,
                                                   HaDiscoveryBridge* haDiscoveryBridge)
    : BaseController(request, action), registry_(registry), adapters_(adapters), haSettingsStore_(haSettingsStore),
      haDiscoveryBridge_(haDiscoveryBridge) {}

#if defined(ARDUINO) && !defined(UNIT_TEST)
void DeviceRegistryController::registerRoutes(AsyncWebServer& server, DeviceRegistry& registry, DeviceScopedDataStore* haSettingsStore,
                                              HaDiscoveryBridge* haDiscoveryBridge) {
    static const DeviceApiAdapterRegistry adapters = DeviceApiAdapterRegistry::withDefaults();
    server.on(AsyncURIMatcher::exact("/api/devices"), HTTP_GET, [&registry, haSettingsStore](AsyncWebServerRequest* request) {
        DeviceRegistryController(request, Action::Index, registry, adapters, haSettingsStore).dispatch();
    });
    server.on(
        AsyncURIMatcher::exact("/api/devices"), HTTP_POST, [&registry](AsyncWebServerRequest* request) { (void)request; }, nullptr,
        [&registry](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            if (!BaseController::appendRequestBody(request, data, len, index, total)) {
                return;
            }

            DeviceRegistryController(request, Action::Create, registry, adapters)
                .dispatch(static_cast<uint8_t*>(request->_tempObject), total);
            BaseController::clearRequestBody(request);
        });
    server.on(AsyncURIMatcher::exact("/api/devices"), HTTP_OPTIONS, [&registry](AsyncWebServerRequest* request) {
        DeviceRegistryController(request, Action::Options, registry, adapters).dispatch();
    });

    server.on(AsyncURIMatcher::exact("/api/devices/flush"), HTTP_POST, [&registry](AsyncWebServerRequest* request) {
        DeviceRegistryController(request, Action::Flush, registry, adapters).dispatch();
    });
    server.on(AsyncURIMatcher::exact("/api/devices/flush"), HTTP_OPTIONS, [&registry](AsyncWebServerRequest* request) {
        DeviceRegistryController(request, Action::Options, registry, adapters).dispatch();
    });

    server.on(AsyncURIMatcher::prefix("/api/devices/"), HTTP_GET, [&registry, haSettingsStore](AsyncWebServerRequest* request) {
        DeviceRegistryController(request, Action::Show, registry, adapters, haSettingsStore).dispatch();
    });
    server.on(AsyncURIMatcher::prefix("/api/devices/"), HTTP_DELETE, [&registry](AsyncWebServerRequest* request) {
        DeviceRegistryController(request, Action::Destroy, registry, adapters).dispatch();
    });
    server.on(
        AsyncURIMatcher::prefix("/api/devices/"), HTTP_POST, [&registry](AsyncWebServerRequest* request) { (void)request; }, nullptr,
        [&registry, haSettingsStore, haDiscoveryBridge](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index,
                                                        size_t total) {
            if (!BaseController::appendRequestBody(request, data, len, index, total)) {
                return;
            }

            DeviceRegistryController(request, Action::Cmd, registry, adapters, haSettingsStore, haDiscoveryBridge)
                .dispatch(static_cast<uint8_t*>(request->_tempObject), total);
            BaseController::clearRequestBody(request);
        });
    server.on(AsyncURIMatcher::prefix("/api/devices/"), HTTP_OPTIONS, [&registry](AsyncWebServerRequest* request) {
        DeviceRegistryController(request, Action::Options, registry, adapters).dispatch();
    });
}
#endif

const BaseController::RulesChain* DeviceRegistryController::beforeChain() {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    static constexpr HookRule rules[] = {
        {&DeviceRegistryController::requireId, A(Action::Show) | A(Action::Destroy) | A(Action::Cmd)},
        {&DeviceRegistryController::requireEntity, A(Action::Show) | A(Action::Destroy) | A(Action::Cmd)},
    };
    static const RulesChain node{rules, sizeof(rules) / sizeof(rules[0]), BaseController::beforeChain()};
    return &node;
#else
    return BaseController::beforeChain();
#endif
}

bool DeviceRegistryController::parseDeviceIdPath(const char* url, const bool requireCommandSuffix, DeviceId& deviceId) {
    constexpr size_t kPrefixLen = sizeof("/api/devices/") - 1;
    if (url == nullptr || std::strlen(url) <= kPrefixLen) {
        return false;
    }

    const char* value = url + kPrefixLen;
    char* end = nullptr;
    const unsigned long parsed = strtoul(value, &end, 10);
    if (end == value || parsed == 0UL) {
        return false;
    }

    if (requireCommandSuffix) {
        if (end == nullptr || std::strcmp(end, "/command") != 0) {
            return false;
        }
    } else if (end != nullptr && *end != '\0') {
        return false;
    }

    deviceId = static_cast<DeviceId>(parsed);
    return true;
}

bool DeviceRegistryController::parseLayoutPath(const char* url, DeviceId& deviceId) {
    constexpr size_t kPrefixLen = sizeof("/api/devices/") - 1;
    if (url == nullptr || std::strlen(url) <= kPrefixLen) {
        return false;
    }
    const char* value = url + kPrefixLen;
    char* end = nullptr;
    const unsigned long parsed = strtoul(value, &end, 10);
    if (end == value || parsed == 0UL || end == nullptr || std::strcmp(end, "/layout") != 0) {
        return false;
    }
    deviceId = static_cast<DeviceId>(parsed);
    return true;
}

bool DeviceRegistryController::requireId(BaseController& self) {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    auto& ctl = static_cast<DeviceRegistryController&>(self); // NOLINT
    if (ctl.request_ == nullptr) {
        return false;
    }

    // GET /api/devices/:id/layout is served by the Show action but has a distinct path suffix.
    if (ctl.action_ == Action::Show && parseLayoutPath(ctl.request_->url().c_str(), ctl.deviceId_)) {
        ctl.layoutRequested_ = true;
        return true;
    }

    const bool requireCommandSuffix = ctl.action_ == Action::Cmd;
    if (!parseDeviceIdPath(ctl.request_->url().c_str(), requireCommandSuffix, ctl.deviceId_)) {
        ctl.renderError(400, "BAD_PARAMS", "bad_id");
        return false;
    }
    return true;
#else
    (void)self;
    return false;
#endif
}

bool DeviceRegistryController::requireEntity(BaseController& self) {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    auto& ctl = static_cast<DeviceRegistryController&>(self); // NOLINT
    if (ctl.deviceId_ == 0U) {
        return false;
    }

    if (ctl.registry_.runtime(ctl.deviceId_) == nullptr) {
        ctl.renderError(404, "NOT_FOUND", "device not found");
        return false;
    }
    return true;
#else
    (void)self;
    return false;
#endif
}

bool DeviceRegistryController::parseCreateAdapter(const JsonVariantConst& json, const char*& error,
                                                  const IDeviceApiAdapter*& adapter) const {
    if (json.isNull()) {
        error = "device payload is missing";
        adapter = nullptr;
        return false;
    }

    const JsonObjectConst object = json.as<JsonObjectConst>();
    const char* typeName = object["typeName"] | "";
    adapter = adapters_.findByName(typeName);
    if (adapter == nullptr) {
        error = "unsupported device type";
    }
    return adapter != nullptr;
}

const char* DeviceRegistryController::statusToString(const DeviceStatus status) {
    switch (status) {
    case DeviceStatus::Creating:
        return "creating";
    case DeviceStatus::Starting:
        return "starting";
    case DeviceStatus::Ready:
        return "ready";
    case DeviceStatus::Disabled:
        return "disabled";
    case DeviceStatus::Faulted:
        return "faulted";
    case DeviceStatus::DependencyBlocked:
        return "dependency_blocked";
    case DeviceStatus::Reconfiguring:
        return "reconfiguring";
    case DeviceStatus::Stopping:
        return "stopping";
    case DeviceStatus::Deleting:
        return "deleting";
    case DeviceStatus::Unknown:
    default:
        return "unknown";
    }
}

const char* DeviceRegistryController::errorCodeForDeviceError(const DeviceError error) {
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
        return "CORRUPT_RECORD";
    case DeviceError::MissingRecord:
        return "NOT_FOUND";
    case DeviceError::InvalidCommand:
        return "INVALID_COMMAND";
    case DeviceError::InvalidConfig:
        return "INVALID_CONFIG";
    case DeviceError::None:
    default:
        return "BAD_ARGS";
    }
}

namespace {
#if defined(ARDUINO) && !defined(UNIT_TEST)
// Sized for the largest single-device JSON. Display layouts are no longer inlined (they load via
// GET /api/devices/:id/layout), so every device now serializes to well under 2 KB - small enough to
// live on the async_tcp task stack (16 KB) as a StaticJsonDocument instead of a heap allocation.
constexpr size_t kDeviceJsonCapacity = 2048;

const char* statusToString(DeviceStatus status) {
    switch (status) {
    case DeviceStatus::Creating:
        return "creating";
    case DeviceStatus::Starting:
        return "starting";
    case DeviceStatus::Ready:
        return "ready";
    case DeviceStatus::Disabled:
        return "disabled";
    case DeviceStatus::Faulted:
        return "faulted";
    case DeviceStatus::DependencyBlocked:
        return "dependency_blocked";
    case DeviceStatus::Reconfiguring:
        return "reconfiguring";
    case DeviceStatus::Stopping:
        return "stopping";
    case DeviceStatus::Deleting:
        return "deleting";
    case DeviceStatus::Unknown:
    default:
        return "unknown";
    }
}

#if defined(WITH_HOME_ASSISTANT)
// Generic (type-agnostic) "ha" block, added alongside record/config/runtime regardless of which
// adapter (or fallback) wrote the rest of the device JSON - no per-type adapter changes needed.
void writeHaDeviceJson(JsonObject device, DeviceScopedDataStore* haSettingsStore, const HaEntityAdapterRegistry* haAdapters,
                       const IDeviceRuntime& runtime) {
    if (haSettingsStore == nullptr) {
        return;
    }
    const bool supported = haAdapters != nullptr && haAdapters->find(runtime.typeId()) != nullptr;
    JsonObject ha = device.createNestedObject("ha");
    ha["supported"] = supported;
    if (!supported) {
        // Device types with no HA adapter (bus/infra devices: SPI/I2C/OneWire bus, displays,
        // dummy) can never have HA settings saved - skip the NVS lookup entirely instead of
        // opening a namespace that will never exist, which otherwise logs a
        // "nvs_open failed: NOT_FOUND" error on every device list fetch for every such device.
        ha["enabled"] = false;
        ha["name"] = "";
        ha["effectiveName"] = JsonString(runtime.name() != nullptr ? runtime.name() : "", JsonString::Copied);
        return;
    }
    const HaDeviceSettingsRecord settings = loadHaDeviceSettings(*haSettingsStore, runtime.deviceId());
    ha["enabled"] = settings.enabled != 0U;
    ha["name"] = JsonString(settings.nameOverride, JsonString::Copied);
    const std::string effectiveName = effectiveHaDeviceName(settings, runtime.name());
    ha["effectiveName"] = JsonString(effectiveName.c_str(), JsonString::Copied);
}
#endif

// Producer state for GET /api/devices, held alive by the chunked-response filler (via shared_ptr)
// because the filler runs on the async_tcp task after index() has already returned. Peak memory is
// bounded to one device's JSON regardless of device count: we serialize one device at a time into a
// reused GrowableBuffer (BaseController::ChunkedBody) and hand it out across as many ~1.4 KB network
// chunks as needed, instead of buffering the whole list in one contiguous, byte-by-byte-grown block
// (AsyncResponseStream, which crashed with std::bad_alloc once the response outgrew the largest free
// heap block).
struct DeviceIndexProducerState {
    DeviceRegistry* registry = nullptr;
    const DeviceApiAdapterRegistry* adapters = nullptr;
    DeviceScopedDataStore* haSettingsStore = nullptr;
    const HaEntityAdapterRegistry* haAdapters = nullptr;
    uint32_t registryRevision = 0;
    bool pendingPersistence = false;

    StaticJsonDocument<kDeviceJsonCapacity> item; // reused per device; inline in this heap-held state
    std::vector<DeviceId> deviceIds;              // snapshot taken up front
    size_t cursor = 0;
    bool anyEmitted = false;
    int phase = 0; // 0 = prefix, 1 = devices, 2 = suffix, 3 = done
};

// Produce the next logical piece into `body`. Returns false only when nothing remains.
bool produceDeviceIndexPiece(DeviceIndexProducerState& s, BaseController::ChunkedBody& body) {
    if (s.phase == 0) {
        char header[96];
        const int n =
            std::snprintf(header, sizeof(header), "{\"success\":true,\"registryRevision\":%lu,\"pendingPersistence\":%s,\"devices\":[",
                          static_cast<unsigned long>(s.registryRevision), s.pendingPersistence ? "true" : "false");
        s.phase = 1;
        return n > 0 && body.emit(header, static_cast<size_t>(n));
    }

    if (s.phase == 1) {
        while (s.cursor < s.deviceIds.size()) {
            const DeviceId id = s.deviceIds[s.cursor++];
            const IDeviceRuntime* runtime = s.registry->runtime(id);
            if (runtime == nullptr) {
                continue; // deleted between snapshot and now - just omit it
            }
            JsonDocument& item = s.item;
            item.clear();
            JsonObject device = item.to<JsonObject>();
            const DeviceStatus effectiveStatus = s.registry->effectiveStatus(id);
            const IDeviceApiAdapter* adapter = s.adapters->find(runtime->typeId());
            if (adapter != nullptr) {
                adapter->writeDeviceJson(*runtime, effectiveStatus, device);
            } else {
                IDeviceApiAdapter::writeFallbackDeviceJson(*runtime, effectiveStatus, "unknown", device);
            }
            device["runtime"]["status"] = statusToString(runtime->status());
            device["runtime"]["effectiveStatus"] = statusToString(effectiveStatus);
#if defined(WITH_HOME_ASSISTANT)
            writeHaDeviceJson(device, s.haSettingsStore, s.haAdapters, *runtime);
#endif
            if (item.overflowed()) {
                EWFM_LOG_WARN("json", "/api/devices skip id=%lu (exceeds %u-byte doc)", static_cast<unsigned long>(id),
                              static_cast<unsigned>(kDeviceJsonCapacity));
                continue; // skip this device rather than emit truncated JSON
            }
            if (!body.emitJson(item, s.anyEmitted)) {
                EWFM_LOG_WARN("heap", "/api/devices skip id=%lu jsonBytes=%u (alloc failed, maxBlock=%lu)", static_cast<unsigned long>(id),
                              static_cast<unsigned>(measureJson(item)), static_cast<unsigned long>(ESP.getMaxAllocHeap()));
                continue; // skip this device rather than abort the whole response
            }
            s.anyEmitted = true;
            return true;
        }
        s.phase = 2; // fall through to suffix
    }

    if (s.phase == 2) {
        s.phase = 3;
        return body.emit("]}", 2);
    }

    return false;
}

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
#endif
} // namespace

void DeviceRegistryController::index() {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    auto state = std::make_shared<DeviceIndexProducerState>();
    state->registry = &registry_;
    state->adapters = &adapters_;
    state->haSettingsStore = haSettingsStore_;
#if defined(WITH_HOME_ASSISTANT)
    // haAdapters_ is a value member of this (temporary) controller, which is destroyed as soon as
    // index() returns - but the chunked producer runs later on the async_tcp task. Point at a stable
    // shared instance instead of the soon-to-dangle member.
    static const HaEntityAdapterRegistry kStreamHaAdapters = HaEntityAdapterRegistry::withDefaults();
    state->haAdapters = &kStreamHaAdapters;
#endif
    state->registryRevision = registry_.registryRevision();
    state->pendingPersistence = registry_.hasPendingPersistence();
    registry_.forEachRuntime([&state](const IDeviceRuntime& runtime) { state->deviceIds.push_back(runtime.deviceId()); });

    sendChunked("application/json", [state](ChunkedBody& body) { return produceDeviceIndexPiece(*state, body); });
#endif
}

void DeviceRegistryController::show() {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    const IDeviceRuntime* runtime = registry_.runtime(deviceId_);
    if (runtime == nullptr) {
        renderError(404, "NOT_FOUND", "device not found");
        return;
    }

    if (layoutRequested_) {
        showLayout(*runtime);
        return;
    }

    StaticJsonDocument<kDeviceJsonCapacity> doc;
    doc["registryRevision"] = registry_.registryRevision();
    doc["pendingPersistence"] = registry_.hasPendingPersistence();
    JsonObject device = doc.createNestedObject("device");
    const DeviceStatus effectiveStatus = registry_.effectiveStatus(runtime->deviceId());
    const IDeviceApiAdapter* adapter = adapters_.find(runtime->typeId());
    if (adapter != nullptr) {
        adapter->writeDeviceJson(*runtime, effectiveStatus, device);
    } else {
        IDeviceApiAdapter::writeFallbackDeviceJson(*runtime, effectiveStatus, "unknown", device);
    }
    device["runtime"]["status"] = statusToString(runtime->status());
    device["runtime"]["effectiveStatus"] = statusToString(effectiveStatus);
#if defined(WITH_HOME_ASSISTANT)
    writeHaDeviceJson(device, haSettingsStore_, &haAdapters_, *runtime);
#endif
    if (doc.overflowed()) {
        renderError(500, "INTERNAL", "device json exceeds buffer");
        return;
    }
    renderOk(doc);
#endif
}

#if defined(ARDUINO) && !defined(UNIT_TEST)
void DeviceRegistryController::showLayout(const IDeviceRuntime& runtime) {
    const IDeviceApiAdapter* adapter = adapters_.find(runtime.typeId());
    int onlyPageIndex = -1;
    if (const AsyncWebParameter* pageParam = request_->getParam("page"); pageParam != nullptr) {
        onlyPageIndex = static_cast<int>(pageParam->value().toInt());
    }
    std::shared_ptr<IJsonChunkProducer> producer = adapter != nullptr ? adapter->createLayoutJsonProducer(runtime, onlyPageIndex) : nullptr;
    if (producer == nullptr) {
        renderError(404, "NOT_FOUND", "device has no display layout");
        return;
    }
    sendChunked("application/json", [producer](ChunkedBody& body) {
        ChunkedBodyJsonSink sink(body);
        return producer->next(sink);
    });
}
#endif

void DeviceRegistryController::create() {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    const char* error = nullptr;
    const JsonObjectConst input = getDoc()->as<JsonObjectConst>();
    const IDeviceApiAdapter* adapter = nullptr;
    if (!parseCreateAdapter(input, error, adapter)) {
        renderError(400, "BAD_ARGS", error);
        return;
    }

    DeviceCreateRequest createRequest;
    if (!adapter->parseCreateRequest(input, createRequest, error)) {
        renderError(400, "BAD_ARGS", error);
        return;
    }
    // Heap-allocated: DeviceCreatePersistenceRequest embeds a BoundedBlob<kMaxDisplayLayoutBytes>
    // (4KB+), which overflowed the async_tcp task's stack when declared as a local here.
    auto createPersistedRequest = std::make_unique<DeviceCreatePersistenceRequest>();
    if (!adapter->parseCreatePersistedStateRequest(input, createRequest, *createPersistedRequest, error)) {
        renderError(400, "BAD_ARGS", error);
        return;
    }
    const DeviceValidationResult createValidation = adapter->validateCreateRequest(createRequest, *createPersistedRequest, registry_);
    if (!createValidation.ok()) {
        renderError(400, errorCodeForDeviceError(createValidation.error), createValidation.message);
        return;
    }

    const DeviceCreateResult result = registry_.command(createRequest, ArduinoClock::millis());
    if (!result.ok()) {
        renderError(400, errorCodeForDeviceError(result.validation.error), result.validation.message);
        return;
    }
    if (createPersistedRequest->persistedStateProvided) {
        const DeviceValidationResult persistedResult = registry_.applyPersistedStateUpdate(
            result.deviceId, createPersistedRequest->persistedStateBlob.data(), createPersistedRequest->persistedStateBlob.size());
        if (!persistedResult.ok()) {
            renderError(400, errorCodeForDeviceError(persistedResult.error), persistedResult.message);
            return;
        }
    }
    StaticJsonDocument<kDeviceJsonCapacity> doc;
    doc["registryRevision"] = registry_.registryRevision();
    doc["pendingPersistence"] = result.pendingPersistence;
    JsonObject device = doc.createNestedObject("device");
    if (const IDeviceRuntime* runtime = registry_.runtime(result.deviceId); runtime != nullptr) {
        const DeviceStatus effectiveStatus = registry_.effectiveStatus(runtime->deviceId());
        adapter->writeDeviceJson(*runtime, effectiveStatus, device);
    }
    if (doc.overflowed()) {
        renderError(500, "INTERNAL", "device json exceeds buffer");
        return;
    }
    sendJson(201, doc);
#endif
}

void DeviceRegistryController::destroy() {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    const uint32_t now = ArduinoClock::millis();

    if (registry_.runtime(deviceId_) == nullptr) {
        renderError(404, "NOT_FOUND", "device not found");
        return;
    }
    const DeviceMutationResult result = registry_.command(DeviceCommand{DeviceCommandType::Delete, deviceId_, ""}, now);
    if (!result.ok()) {
        StaticJsonDocument<384> doc;
        doc["success"] = false;
        doc["code"] = errorCodeForDeviceError(result.validation.error);
        doc["error"] = result.validation.message;
        if (result.validation.error == DeviceError::InvalidRelationship && !result.dependentDeviceIds.empty()) {
            doc["code"] = "DEPENDENT_DELETE";
            JsonArray ids = doc.createNestedArray("dependentDeviceIds");
            for (const DeviceId childId : result.dependentDeviceIds) {
                ids.add(childId);
            }
        }
        sendJson(400, doc);
        return;
    }

    StaticJsonDocument<256> doc;
    doc["registryRevision"] = registry_.registryRevision();
    doc["pendingPersistence"] = result.pendingPersistence;
    renderOk(doc);
#endif
}

void DeviceRegistryController::cmd() {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    const IDeviceRuntime* currentRuntime = registry_.runtime(deviceId_);
    if (currentRuntime == nullptr) {
        renderError(404, "NOT_FOUND", "device not found");
        return;
    }
    const JsonObjectConst input = getDoc()->as<JsonObjectConst>();
    const DeviceId bodyDeviceId = static_cast<DeviceId>(input["deviceId"] | 0U);
    if (bodyDeviceId != 0U && bodyDeviceId != deviceId_) {
        renderError(400, "BAD_ARGS", "deviceId mismatch");
        return;
    }

    const char* commandName = input["command"] | "";
    if (commandName == nullptr || *commandName == '\0') {
        renderError(400, "BAD_ARGS", "unsupported command");
        return;
    }

    DeviceMutationResult mutationResult{};
    const uint32_t now = ArduinoClock::millis();
    if (std::strcmp(commandName, "delete") == 0) {
        mutationResult = registry_.command(DeviceCommand{DeviceCommandType::Delete, deviceId_, ""}, now);
        if (!mutationResult.ok()) {
            renderError(400, errorCodeForDeviceError(mutationResult.validation.error), mutationResult.validation.message);
            return;
        }
    } else if (std::strcmp(commandName, "resetDiagnostics") == 0) {
        mutationResult = registry_.command(DeviceCommand{DeviceCommandType::ResetDiagnostics, deviceId_, ""}, now);
        if (!mutationResult.ok()) {
            renderError(400, errorCodeForDeviceError(mutationResult.validation.error), mutationResult.validation.message);
            return;
        }
    } else if (std::strcmp(commandName, "scan") == 0) {
        mutationResult = registry_.command(DeviceCommand{DeviceCommandType::Scan, deviceId_, ""}, now);
        if (!mutationResult.ok()) {
            renderError(400, errorCodeForDeviceError(mutationResult.validation.error), mutationResult.validation.message);
            return;
        }
    } else if (std::strcmp(commandName, "checkDevice") == 0) {
        const int csPinRaw = input["csPin"] | -1;
        if (csPinRaw < 0 || csPinRaw > 39) {
            renderError(400, "BAD_ARGS", "csPin is required and must be in range [0, 39]");
            return;
        }
        char csPinText[8]{};
        std::snprintf(csPinText, sizeof(csPinText), "%d", csPinRaw);
        mutationResult = registry_.command(DeviceCommand{DeviceCommandType::CheckDevice, deviceId_, csPinText}, now);
        if (!mutationResult.ok()) {
            renderError(400, errorCodeForDeviceError(mutationResult.validation.error), mutationResult.validation.message);
            return;
        }
    } else if (std::strcmp(commandName, "setOutput") == 0) {
        char payloadText[16]{};
        const JsonVariantConst state = input["state"];
        if (currentRuntime->analogOutputRuntime() != nullptr) {
            if ((!state.is<unsigned long>() && !state.is<long>() && !state.is<int>()) || state.as<long>() < 0L ||
                state.as<unsigned long>() > 100UL) {
                renderError(400, "BAD_ARGS", "state must be numeric and in range [0, 100]");
                return;
            }
            std::snprintf(payloadText, sizeof(payloadText), "%lu", state.as<unsigned long>());
        } else if (currentRuntime->switchOutputRuntime() != nullptr) {
            if (!state.is<bool>()) {
                renderError(400, "BAD_ARGS", "state must be boolean");
                return;
            }
            std::snprintf(payloadText, sizeof(payloadText), "%s", state.as<bool>() ? "true" : "false");
        } else {
            renderError(400, "BAD_ARGS", "device does not provide an output");
            return;
        }
        mutationResult = registry_.command(DeviceCommand{DeviceCommandType::SetOutput, deviceId_, payloadText}, now);
        if (!mutationResult.ok()) {
            renderError(400, errorCodeForDeviceError(mutationResult.validation.error), mutationResult.validation.message);
            return;
        }
    } else if (std::strcmp(commandName, "setDeps") == 0) {
        const IDeviceApiAdapter* adapter = adapters_.find(currentRuntime->typeId());
        if (adapter == nullptr) {
            renderError(400, "BAD_ARGS", "unsupported device type");
            return;
        }
        DeviceCommand::DepsPayload depsPayload{};
        const char* depsError = nullptr;
        if (!adapter->parseSetDepsRequest(input, depsPayload.deps, depsPayload.depCount, depsError)) {
            renderError(400, "BAD_ARGS", depsError != nullptr ? depsError : "deps are invalid");
            return;
        }
        const IDeviceRuntime* runtime = registry_.runtime(deviceId_);
        if (runtime != nullptr) {
            const DeviceValidationResult depsValidation =
                adapter->validateSetDepsRequest(*runtime, depsPayload.deps, depsPayload.depCount, registry_);
            if (!depsValidation.ok()) {
                renderError(400, errorCodeForDeviceError(depsValidation.error), depsValidation.message);
                return;
            }
        }
        mutationResult = registry_.command(DeviceCommand{DeviceCommandType::SetDeps, deviceId_, depsPayload}, now);
        if (!mutationResult.ok()) {
            renderError(400, errorCodeForDeviceError(mutationResult.validation.error), mutationResult.validation.message);
            return;
        }
    } else if (std::strcmp(commandName, "updateConfig") == 0) {
        const IDeviceApiAdapter* adapter = adapters_.find(currentRuntime->typeId());
        if (adapter == nullptr) {
            renderError(400, "BAD_ARGS", "unsupported device type");
            return;
        }
        const char* error = nullptr;
        // Heap-allocated: DeviceConfigUpdateRequest embeds a BoundedBlob<kMaxDisplayLayoutBytes>
        // (4KB+), too large to declare as a local on the async_tcp task's stack (see the matching
        // fix in create() above for the same struct family and the stack overflow it caused there).
        auto updateRequest = std::make_unique<DeviceConfigUpdateRequest>();
        IDeviceRuntime* runtime = registry_.runtime(deviceId_);
        if (runtime == nullptr) {
            renderError(404, "NOT_FOUND", "device not found");
            return;
        }
        if (!adapter->parseUpdateConfigRequest(input, *runtime, *updateRequest, error)) {
            renderError(400, "BAD_ARGS", error);
            return;
        }
        const DeviceValidationResult updateValidation = adapter->validateUpdateConfigRequest(*runtime, *updateRequest, registry_);
        if (!updateValidation.ok()) {
            renderError(400, errorCodeForDeviceError(updateValidation.error), updateValidation.message);
            return;
        }
        mutationResult =
            registry_.updateConfigAndDeps(deviceId_, updateRequest->configBlob, updateRequest->configVersion, updateRequest->baseConfig,
                                          updateRequest->depsProvided, updateRequest->deps, updateRequest->depCount, now);
        if (!mutationResult.ok()) {
            renderError(400, errorCodeForDeviceError(mutationResult.validation.error), mutationResult.validation.message);
            return;
        }
        if (updateRequest->persistedStateProvided) {
            const DeviceValidationResult persistedResult = registry_.applyPersistedStateUpdate(
                deviceId_, updateRequest->persistedStateBlob.data(), updateRequest->persistedStateBlob.size());
            if (!persistedResult.ok()) {
                renderError(400, errorCodeForDeviceError(persistedResult.error), persistedResult.message);
                return;
            }
        }
    } else if (std::strcmp(commandName, "setHaSettings") == 0) {
#if defined(WITH_HOME_ASSISTANT)
        if (haSettingsStore_ == nullptr) {
            renderError(400, "BAD_ARGS", "home assistant integration is not available");
            return;
        }
        const bool haEnabled = input["haEnabled"] | false;
        const char* haName = input["haName"] | "";
        if (haEnabled && haAdapters_.find(currentRuntime->typeId()) == nullptr) {
            renderError(400, "BAD_ARGS", "device type does not support Home Assistant integration");
            return;
        }
        const DeviceValidationResult saveResult = saveHaDeviceSettings(*haSettingsStore_, deviceId_, haEnabled, haName);
        if (!saveResult.ok()) {
            renderError(400, errorCodeForDeviceError(saveResult.error), saveResult.message);
            return;
        }
        if (haDiscoveryBridge_ != nullptr) {
            haDiscoveryBridge_->refreshDevice(deviceId_);
        }
        mutationResult = DeviceMutationResult{};
#else
        renderError(400, "BAD_ARGS", "unsupported command");
        return;
#endif
    } else if (std::strcmp(commandName, "startDose") == 0) {
        // Dosing pump: amountMl (float) + logging flag composed into the device's Custom-command
        // grammar ("startDose:<centiMl>:<0|1>"); logging=false marks a calibration run that is
        // excluded from totals and the journal.
        const double amountMl = input["amountMl"] | 0.0;
        const bool logging = input["logging"] | true;
        const long amountCentiMl = std::lround(amountMl * 100.0);
        if (amountCentiMl < 1L || amountCentiMl > 65535L) {
            renderError(400, "BAD_ARGS", "amountMl is required and must be 0.01-655.35");
            return;
        }
        char payload[32]{};
        std::snprintf(payload, sizeof(payload), "startDose:%ld:%d", amountCentiMl, logging ? 1 : 0);
        mutationResult = registry_.command(DeviceCommand{DeviceCommandType::Custom, deviceId_, payload}, now);
        if (!mutationResult.ok()) {
            renderError(400, errorCodeForDeviceError(mutationResult.validation.error), mutationResult.validation.message);
            return;
        }
    } else if (std::strcmp(commandName, "stopDose") == 0) {
        mutationResult = registry_.command(DeviceCommand{DeviceCommandType::Custom, deviceId_, "stopDose"}, now);
        if (!mutationResult.ok()) {
            renderError(400, errorCodeForDeviceError(mutationResult.validation.error), mutationResult.validation.message);
            return;
        }
    } else if (std::strcmp(commandName, "setVolume") == 0) {
        // Dosing pump container refill/correction: the user reports the volume now in the container.
        const double volumeMl = input["volumeMl"] | -1.0;
        const long volumeCentiMl = std::lround(volumeMl * 100.0);
        if (volumeMl < 0.0 || volumeCentiMl < 0L) {
            renderError(400, "BAD_ARGS", "volumeMl is required and must be non-negative");
            return;
        }
        char payload[32]{};
        std::snprintf(payload, sizeof(payload), "setVolume:%ld", volumeCentiMl);
        mutationResult = registry_.command(DeviceCommand{DeviceCommandType::Custom, deviceId_, payload}, now);
        if (!mutationResult.ok()) {
            renderError(400, errorCodeForDeviceError(mutationResult.validation.error), mutationResult.validation.message);
            return;
        }
    } else if (std::strcmp(commandName, "skipNext") == 0) {
        const int doseIndex = input["doseIndex"] | -1;
        const bool skip = input["skip"] | true;
        if (doseIndex < 0 || doseIndex >= static_cast<int>(kMaxDosingPumpDoses)) {
            renderError(400, "BAD_ARGS", "doseIndex is required and must address a schedule dose");
            return;
        }
        char payload[32]{};
        std::snprintf(payload, sizeof(payload), "skipNext:%d:%d", doseIndex, skip ? 1 : 0);
        mutationResult = registry_.command(DeviceCommand{DeviceCommandType::Custom, deviceId_, payload}, now);
        if (!mutationResult.ok()) {
            renderError(400, errorCodeForDeviceError(mutationResult.validation.error), mutationResult.validation.message);
            return;
        }
    } else if (std::strcmp(commandName, "setMode") == 0) {
        // Generic free-text Custom-command bridge (e.g. AutoSwitchDevice's "auto"/"pause" mode
        // commands) - the device runtime itself defines which payload strings it accepts via
        // handleCommand(), the controller only forwards the raw string through.
        const char* modeValue = input["mode"] | "";
        if (*modeValue == '\0') {
            renderError(400, "BAD_ARGS", "mode is required");
            return;
        }
        mutationResult = registry_.command(DeviceCommand{DeviceCommandType::Custom, deviceId_, modeValue}, now);
        if (!mutationResult.ok()) {
            renderError(400, errorCodeForDeviceError(mutationResult.validation.error), mutationResult.validation.message);
            return;
        }
    } else {
        renderError(400, "BAD_ARGS", "unsupported command");
        return;
    }

    StaticJsonDocument<kDeviceJsonCapacity> doc;
    doc["registryRevision"] = registry_.registryRevision();
    doc["pendingPersistence"] = mutationResult.pendingPersistence;
    if (const IDeviceRuntime* runtime = registry_.runtime(deviceId_); runtime != nullptr) {
        JsonObject device = doc.createNestedObject("device");
        const IDeviceApiAdapter* adapter = adapters_.find(runtime->typeId());
        const DeviceStatus effectiveStatus = registry_.effectiveStatus(runtime->deviceId());
        if (adapter != nullptr) {
            adapter->writeDeviceJson(*runtime, effectiveStatus, device);
        } else {
            IDeviceApiAdapter::writeFallbackDeviceJson(*runtime, effectiveStatus, "unknown", device);
        }
        device["runtime"]["status"] = statusToString(runtime->status());
        device["runtime"]["effectiveStatus"] = statusToString(effectiveStatus);
#if defined(WITH_HOME_ASSISTANT)
        writeHaDeviceJson(device, haSettingsStore_, &haAdapters_, *runtime);
#endif
    }
    if (doc.overflowed()) {
        renderError(500, "INTERNAL", "device json exceeds buffer");
        return;
    }
    renderOk(doc);
#endif
}

void DeviceRegistryController::flush() {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    const DeviceValidationResult result = registry_.flushNow();
    if (!result.ok()) {
        renderError(500, "STORAGE_ERROR", result.message);
        return;
    }

    StaticJsonDocument<256> doc;
    doc["registryRevision"] = registry_.registryRevision();
    doc["pendingPersistence"] = registry_.hasPendingPersistence();
    renderOk(doc);
#endif
}

void DeviceRegistryController::options() {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    BaseController::options();
#endif
}

} // namespace ewfm
