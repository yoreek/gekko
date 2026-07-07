#include "portal/controllers/DeviceRegistryController.h"

#include "debug/Debug.h"
#include "devices/core/DeviceBaseConfig.h"
#include "integrations/mqtt/HaDeviceSettings.h"

#if defined(WITH_HOME_ASSISTANT)
#include "integrations/mqtt/HaDiscoveryBridge.h"
#endif

#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>

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

bool DeviceRegistryController::requireId(BaseController& self) {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    auto& ctl = static_cast<DeviceRegistryController&>(self); // NOLINT
    if (ctl.request_ == nullptr) {
        return false;
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

void writeFallbackDeviceJson(JsonObject device, const IDeviceRuntime& runtime, const DeviceStatus effectiveStatus, const char* typeName) {
    JsonObject record = device.createNestedObject("record");
    record["id"] = runtime.deviceId();
    record["typeName"] = typeName;
    record["configRevision"] = runtime.configRevision();

    JsonObject config = device.createNestedObject("config");
    config["name"] = JsonString(runtime.name() != nullptr ? runtime.name() : "", JsonString::Copied);
    config["enabled"] = runtime.enabled();
    JsonArray deps = config.createNestedArray("deps");
    const DeviceDependencyLink* dependencyLinks = runtime.dependencyLinks();
    const uint8_t dependencyCount = runtime.dependencyCount();
    for (uint8_t index = 0; index < dependencyCount && dependencyLinks != nullptr; ++index) {
        JsonObject item = deps.createNestedObject();
        item["role"] = deviceDependencyRoleName(dependencyLinks[index].role);
        item["deviceId"] = dependencyLinks[index].deviceId;
    }

    JsonObject runtimeJson = device.createNestedObject("runtime");
    runtimeJson["status"] = statusToString(runtime.status());
    runtimeJson["effectiveStatus"] = statusToString(effectiveStatus);
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

void writeIndexResponse(AsyncResponseStream* response, DeviceRegistry& registry, const DeviceApiAdapterRegistry& adapters,
                        DeviceScopedDataStore* haSettingsStore, const HaEntityAdapterRegistry* haAdapters) {
    (void)haSettingsStore;
    (void)haAdapters;
    if (response == nullptr) {
        return;
    }

    response->print("{\"success\":true,");
    response->printf("\"registryRevision\":%lu,", static_cast<unsigned long>(registry.registryRevision()));
    response->printf("\"pendingPersistence\":%s,", registry.hasPendingPersistence() ? "true" : "false");
    response->print("\"devices\":[");

    bool first = true;
    registry.forEachRuntime([&](const IDeviceRuntime& runtime) {
        if (!first) {
            response->print(',');
        }
        first = false;

        StaticJsonDocument<8192> item;
        JsonObject device = item.to<JsonObject>();
        const DeviceStatus effectiveStatus = registry.effectiveStatus(runtime.deviceId());
        const IDeviceApiAdapter* adapter = adapters.find(runtime.typeId());
        if (adapter != nullptr) {
            adapter->writeDeviceJson(runtime, effectiveStatus, device);
        } else {
            writeFallbackDeviceJson(device, runtime, effectiveStatus, "unknown");
        }
        const DeviceStatus lifecycleStatus = runtime.status();
        device["runtime"]["status"] = statusToString(lifecycleStatus);
        device["runtime"]["effectiveStatus"] = statusToString(effectiveStatus);
#if defined(WITH_HOME_ASSISTANT)
        writeHaDeviceJson(device, haSettingsStore, haAdapters, runtime);
#endif
        serializeJson(item, *response);
    });

    response->print("]}");
}
#endif
} // namespace

void DeviceRegistryController::index() {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    AsyncResponseStream* response = request_->beginResponseStream("application/json");
#if defined(WITH_HOME_ASSISTANT)
    writeIndexResponse(response, registry_, adapters_, haSettingsStore_, &haAdapters_);
#else
    writeIndexResponse(response, registry_, adapters_, haSettingsStore_, nullptr);
#endif
    send(response);
#endif
}

void DeviceRegistryController::show() {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    const IDeviceRuntime* runtime = registry_.runtime(deviceId_);
    if (runtime == nullptr) {
        renderError(404, "NOT_FOUND", "device not found");
        return;
    }

    StaticJsonDocument<8192> doc;
    doc["registryRevision"] = registry_.registryRevision();
    doc["pendingPersistence"] = registry_.hasPendingPersistence();
    JsonObject device = doc.createNestedObject("device");
    const DeviceStatus effectiveStatus = registry_.effectiveStatus(runtime->deviceId());
    const IDeviceApiAdapter* adapter = adapters_.find(runtime->typeId());
    if (adapter != nullptr) {
        adapter->writeDeviceJson(*runtime, effectiveStatus, device);
    } else {
        writeFallbackDeviceJson(device, *runtime, effectiveStatus, "unknown");
    }
    device["runtime"]["status"] = statusToString(runtime->status());
    device["runtime"]["effectiveStatus"] = statusToString(effectiveStatus);
#if defined(WITH_HOME_ASSISTANT)
    writeHaDeviceJson(device, haSettingsStore_, &haAdapters_, *runtime);
#endif
    renderOk(doc);
#endif
}

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

    const DeviceCreateResult result = registry_.command(createRequest, 0);
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
    StaticJsonDocument<8192> doc;
    doc["registryRevision"] = registry_.registryRevision();
    doc["pendingPersistence"] = result.pendingPersistence;
    JsonObject device = doc.createNestedObject("device");
    if (const IDeviceRuntime* runtime = registry_.runtime(result.deviceId); runtime != nullptr) {
        const DeviceStatus effectiveStatus = registry_.effectiveStatus(runtime->deviceId());
        adapter->writeDeviceJson(*runtime, effectiveStatus, device);
    }
    sendJson(201, doc);
#endif
}

void DeviceRegistryController::destroy() {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    if (registry_.runtime(deviceId_) == nullptr) {
        renderError(404, "NOT_FOUND", "device not found");
        return;
    }
    const DeviceMutationResult result = registry_.command(DeviceCommand{DeviceCommandType::Delete, deviceId_, ""}, 0);
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
    if (std::strcmp(commandName, "delete") == 0) {
        mutationResult = registry_.command(DeviceCommand{DeviceCommandType::Delete, deviceId_, ""}, 0);
        if (!mutationResult.ok()) {
            renderError(400, errorCodeForDeviceError(mutationResult.validation.error), mutationResult.validation.message);
            return;
        }
    } else if (std::strcmp(commandName, "resetDiagnostics") == 0) {
        mutationResult = registry_.command(DeviceCommand{DeviceCommandType::ResetDiagnostics, deviceId_, ""}, 0);
        if (!mutationResult.ok()) {
            renderError(400, errorCodeForDeviceError(mutationResult.validation.error), mutationResult.validation.message);
            return;
        }
    } else if (std::strcmp(commandName, "scan") == 0) {
        mutationResult = registry_.command(DeviceCommand{DeviceCommandType::Scan, deviceId_, ""}, 0);
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
        mutationResult = registry_.command(DeviceCommand{DeviceCommandType::CheckDevice, deviceId_, csPinText}, 0);
        if (!mutationResult.ok()) {
            renderError(400, errorCodeForDeviceError(mutationResult.validation.error), mutationResult.validation.message);
            return;
        }
    } else if (std::strcmp(commandName, "setOutput") == 0) {
        const char* state = input["state"] | "";
        if (*state == '\0') {
            renderError(400, "BAD_ARGS", "state is required");
            return;
        }
        mutationResult = registry_.command(DeviceCommand{DeviceCommandType::SetOutput, deviceId_, state}, 0);
        if (!mutationResult.ok()) {
            renderError(400, errorCodeForDeviceError(mutationResult.validation.error), mutationResult.validation.message);
            return;
        }
    } else if (std::strcmp(commandName, "setDeps") == 0) {
        const JsonArrayConst depsArray = input["deps"].as<JsonArrayConst>();
        if (depsArray.isNull()) {
            renderError(400, "BAD_ARGS", "deps are required");
            return;
        }
        DeviceCommand::DepsPayload depsPayload{};
        uint8_t depCount = 0;
        for (JsonObjectConst item : depsArray) {
            if (depCount >= kMaxDeviceDependencies) {
                renderError(400, "BAD_ARGS", "deps exceed supported count");
                return;
            }
            DeviceDependencyRole role{DeviceDependencyRole::Unknown};
            if (!parseDeviceDependencyRole(item["role"] | "", role)) {
                renderError(400, "BAD_ARGS", "dependency role is invalid");
                return;
            }
            const DeviceId dependencyDeviceId = static_cast<DeviceId>(item["deviceId"] | 0U);
            if (dependencyDeviceId == 0U) {
                renderError(400, "BAD_ARGS", "dependency deviceId is required");
                return;
            }
            depsPayload.deps[depCount++] = DeviceDependencyLink{role, dependencyDeviceId};
        }
        depsPayload.depCount = depCount;
        const IDeviceApiAdapter* adapter = adapters_.find(currentRuntime->typeId());
        const IDeviceRuntime* runtime = registry_.runtime(deviceId_);
        if (adapter != nullptr && runtime != nullptr) {
            const DeviceValidationResult depsValidation =
                adapter->validateSetDepsRequest(*runtime, depsPayload.deps, depsPayload.depCount, registry_);
            if (!depsValidation.ok()) {
                renderError(400, errorCodeForDeviceError(depsValidation.error), depsValidation.message);
                return;
            }
        }
        mutationResult = registry_.command(DeviceCommand{DeviceCommandType::SetDeps, deviceId_, depsPayload}, 0);
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
        mutationResult = registry_.updateConfigAndDeps(deviceId_, updateRequest->configBlob, updateRequest->configVersion,
                                                       updateRequest->name, updateRequest->enabled, updateRequest->depsProvided,
                                                       updateRequest->deps, updateRequest->depCount, 0);
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
    } else {
        renderError(400, "BAD_ARGS", "unsupported command");
        return;
    }

    StaticJsonDocument<8192> doc;
    doc["registryRevision"] = registry_.registryRevision();
    doc["pendingPersistence"] = mutationResult.pendingPersistence;
    if (const IDeviceRuntime* runtime = registry_.runtime(deviceId_); runtime != nullptr) {
        JsonObject device = doc.createNestedObject("device");
        const IDeviceApiAdapter* adapter = adapters_.find(runtime->typeId());
        const DeviceStatus effectiveStatus = registry_.effectiveStatus(runtime->deviceId());
        if (adapter != nullptr) {
            adapter->writeDeviceJson(*runtime, effectiveStatus, device);
        } else {
            writeFallbackDeviceJson(device, *runtime, effectiveStatus, "unknown");
        }
        device["runtime"]["status"] = statusToString(runtime->status());
        device["runtime"]["effectiveStatus"] = statusToString(effectiveStatus);
#if defined(WITH_HOME_ASSISTANT)
        writeHaDeviceJson(device, haSettingsStore_, &haAdapters_, *runtime);
#endif
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
