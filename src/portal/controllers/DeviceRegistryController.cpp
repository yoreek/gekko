#include "portal/controllers/DeviceRegistryController.h"

#include <cstdlib>
#include <cstring>

#if defined(ARDUINO) && !defined(UNIT_TEST)
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#endif

namespace ewfm {

#if defined(ARDUINO) && !defined(UNIT_TEST)
namespace {
bool appendRequestBody(AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
    if (request == nullptr) {
        return false;
    }

    if (index == 0U) {
        if (request->_tempObject != nullptr) {
            free(request->_tempObject);
            request->_tempObject = nullptr;
        }

        request->_tempObject = calloc(total + 1U, sizeof(uint8_t));
        if (request->_tempObject == nullptr) {
            return false;
        }
    }

    if (request->_tempObject == nullptr) {
        return false;
    }

    auto* buffer = static_cast<uint8_t*>(request->_tempObject);
    std::memcpy(buffer + index, data, len);
    return (index + len) == total;
}

void clearRequestBody(AsyncWebServerRequest* request) {
    if (request == nullptr || request->_tempObject == nullptr) {
        return;
    }

    free(request->_tempObject);
    request->_tempObject = nullptr;
}
} // namespace
#endif

DeviceRegistryController::DeviceRegistryController(AsyncWebServerRequest* request, const Action action, DeviceRegistry& registry,
                                                   const DeviceApiAdapterRegistry& adapters)
    : BaseController(request, action), registry_(registry), adapters_(adapters) {}

#if defined(ARDUINO) && !defined(UNIT_TEST)
void DeviceRegistryController::registerRoutes(AsyncWebServer& server, DeviceRegistry& registry) {
    static const DeviceApiAdapterRegistry adapters = DeviceApiAdapterRegistry::withDefaults();
    server.on(AsyncURIMatcher::exact("/api/devices"), HTTP_GET, [&registry](AsyncWebServerRequest* request) {
        DeviceRegistryController(request, Action::Index, registry, adapters).dispatch();
    });
    server.on(
        AsyncURIMatcher::exact("/api/devices"), HTTP_POST, [&registry](AsyncWebServerRequest* request) { (void)request; }, nullptr,
        [&registry](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            if (!appendRequestBody(request, data, len, index, total)) {
                return;
            }

            DeviceRegistryController(request, Action::Create, registry, adapters)
                .dispatch(static_cast<uint8_t*>(request->_tempObject), total);
            clearRequestBody(request);
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

    server.on(AsyncURIMatcher::prefix("/api/devices/"), HTTP_GET, [&registry](AsyncWebServerRequest* request) {
        DeviceRegistryController(request, Action::Show, registry, adapters).dispatch();
    });
    server.on(AsyncURIMatcher::prefix("/api/devices/"), HTTP_DELETE, [&registry](AsyncWebServerRequest* request) {
        DeviceRegistryController(request, Action::Destroy, registry, adapters).dispatch();
    });
    server.on(
        AsyncURIMatcher::prefix("/api/devices/"), HTTP_POST, [&registry](AsyncWebServerRequest* request) { (void)request; }, nullptr,
        [&registry](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            if (!appendRequestBody(request, data, len, index, total)) {
                return;
            }

            DeviceRegistryController(request, Action::Cmd, registry, adapters).dispatch(static_cast<uint8_t*>(request->_tempObject), total);
            clearRequestBody(request);
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

    ctl.record_ = ctl.registry_.find(ctl.deviceId_);
    if (ctl.record_ == nullptr) {
        ctl.renderError(404, "NOT_FOUND", "device not found");
        return false;
    }
    return true;
#else
    (void)self;
    return false;
#endif
}

bool DeviceRegistryController::parseCreateAdapter(const JsonVariantConst& json, std::string& error,
                                                  const IDeviceApiAdapter*& adapter) const {
    if (json.isNull()) {
        error = "device payload is missing";
        adapter = nullptr;
        return false;
    }

    const JsonObjectConst object = json.as<JsonObjectConst>();
    const uint32_t typeId = object["type_id"] | 0U;
    if (typeId != 0U) {
        adapter = adapters_.find(static_cast<DeviceTypeId>(typeId));
        if (adapter == nullptr) {
            error = "unsupported device type";
        }
        return adapter != nullptr;
    }

    const char* typeName = object["type"] | "";
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

DevicePersistencePolicy DeviceRegistryController::parsePolicy(const JsonObjectConst& input) {
    const char* value = input["persistence_policy"] | "immediate";
    if (std::strcmp(value, "delayed") == 0) {
        return DevicePersistencePolicy::Delayed;
    }
    if (std::strcmp(value, "coalesced") == 0) {
        return DevicePersistencePolicy::Coalesced;
    }
    return DevicePersistencePolicy::Immediate;
}

bool DeviceRegistryController::parseCommandType(const char* value, DeviceCommandType& type) {
    if (value == nullptr || *value == '\0') {
        return false;
    }
    if (std::strcmp(value, "rename") == 0) {
        type = DeviceCommandType::Rename;
        return true;
    }
    if (std::strcmp(value, "enable") == 0) {
        type = DeviceCommandType::Enable;
        return true;
    }
    if (std::strcmp(value, "disable") == 0) {
        type = DeviceCommandType::Disable;
        return true;
    }
    if (std::strcmp(value, "delete") == 0) {
        type = DeviceCommandType::Delete;
        return true;
    }
    if (std::strcmp(value, "update_config") == 0) {
        type = DeviceCommandType::UpdateConfig;
        return true;
    }
    if (std::strcmp(value, "set_status") == 0) {
        type = DeviceCommandType::SetStatus;
        return true;
    }
    if (std::strcmp(value, "custom") == 0) {
        type = DeviceCommandType::Custom;
        return true;
    }
    if (std::strcmp(value, "set_parent") == 0) {
        type = DeviceCommandType::SetParent;
        return true;
    }
    return false;
}

void DeviceRegistryController::index() {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    AsyncResponseStream* response = request_->beginResponseStream("application/json");
    response->print("{\"success\":true,");
    response->printf("\"registry_revision\":%lu,", static_cast<unsigned long>(registry_.registryRevision()));
    response->printf("\"pending_persistence\":%s,", registry_.hasPendingPersistence() ? "true" : "false");
    response->print("\"devices\":[");
    bool first = true;
    for (const auto& record : registry_.list()) {
        const IDeviceApiAdapter* adapter = adapters_.find(record.header.typeId);
        if (!first) {
            response->print(',');
        }
        first = false;
        StaticJsonDocument<512> item;
        JsonObject device = item.to<JsonObject>();
        const IDeviceRuntime* runtime = registry_.runtime(record.header.deviceId);
        if (adapter != nullptr) {
            adapter->writeDeviceJson(record, runtime, device);
        } else {
            device["device_id"] = record.header.deviceId;
            device["type_id"] = record.header.typeId;
            device["name"] = record.name;
            device["enabled"] = record.enabled;
        }
        const DeviceRecord* persisted = registry_.find(record.header.deviceId);
        const DeviceStatus lifecycleStatus =
            runtime != nullptr ? runtime->status() : (persisted != nullptr ? persisted->status : record.status);
        device["device_id"] = record.header.deviceId;
        device["type_id"] = record.header.typeId;
        device["name"] = record.name;
        device["enabled"] = record.enabled;
        device["has_parent"] = record.hasParent;
        device["parent_device_id"] = record.parentDeviceId;
        device["config_version"] = record.header.configVersion;
        device["config_revision"] = record.header.configRevision;
        device["lifecycle_status"] = statusToString(lifecycleStatus);
        device["effective_status"] = statusToString(record.status);
        device["registry_revision"] = registry_.registryRevision();
        device["pending_persistence"] = registry_.hasPendingPersistence();
        serializeJson(item, *response);
    }
    response->print("]}");
    send(response);
#endif
}

void DeviceRegistryController::show() {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    if (record_ == nullptr) {
        renderError(404, "NOT_FOUND", "device not found");
        return;
    }

    StaticJsonDocument<1024> doc;
    doc["registry_revision"] = registry_.registryRevision();
    doc["pending_persistence"] = registry_.hasPendingPersistence();
    JsonObject device = doc.createNestedObject("device");
    const IDeviceApiAdapter* adapter = adapters_.find(record_->header.typeId);
    DeviceRecord effectiveRecord = *record_;
    effectiveRecord.status = registry_.effectiveStatus(record_->header.deviceId);
    const IDeviceRuntime* runtime = registry_.runtime(effectiveRecord.header.deviceId);
    if (adapter != nullptr) {
        adapter->writeDeviceJson(effectiveRecord, runtime, device);
    } else {
        device["device_id"] = effectiveRecord.header.deviceId;
        device["type_id"] = effectiveRecord.header.typeId;
        device["name"] = effectiveRecord.name;
        device["enabled"] = effectiveRecord.enabled;
    }
    const DeviceStatus lifecycleStatus = runtime != nullptr ? runtime->status() : record_->status;
    device["device_id"] = effectiveRecord.header.deviceId;
    device["type_id"] = effectiveRecord.header.typeId;
    device["name"] = effectiveRecord.name;
    device["enabled"] = effectiveRecord.enabled;
    device["has_parent"] = effectiveRecord.hasParent;
    device["parent_device_id"] = effectiveRecord.parentDeviceId;
    device["config_version"] = effectiveRecord.header.configVersion;
    device["config_revision"] = effectiveRecord.header.configRevision;
    device["lifecycle_status"] = statusToString(lifecycleStatus);
    device["effective_status"] = statusToString(effectiveRecord.status);
    device["registry_revision"] = registry_.registryRevision();
    device["pending_persistence"] = registry_.hasPendingPersistence();
    renderOk(doc);
#endif
}

void DeviceRegistryController::create() {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    std::string error;
    const JsonObjectConst input = getDoc()->as<JsonObjectConst>();
    const IDeviceApiAdapter* adapter = nullptr;
    if (!parseCreateAdapter(input, error, adapter)) {
        renderError(400, "BAD_ARGS", error.c_str());
        return;
    }

    DeviceCreateRequest createRequest;
    if (!adapter->parseCreateRequest(input, createRequest, error)) {
        renderError(400, "BAD_ARGS", error.c_str());
        return;
    }

    const DeviceCreateResult result = registry_.command(createRequest, 0);
    if (!result.ok()) {
        renderError(400, errorCodeForDeviceError(result.validation.error), result.validation.message);
        return;
    }

    const DeviceRecord* record = registry_.find(result.deviceId);
    if (record == nullptr) {
        renderError(500, "INTERNAL", "created device not found");
        return;
    }

    StaticJsonDocument<1024> doc;
    doc["registry_revision"] = registry_.registryRevision();
    doc["pending_persistence"] = result.pendingPersistence;
    JsonObject device = doc.createNestedObject("device");
    adapter->writeDeviceJson(*record, registry_.runtime(record->header.deviceId), device);
    sendJson(201, doc);
#endif
}

void DeviceRegistryController::destroy() {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    const DeviceMutationResult result =
        registry_.command(DeviceCommand{DeviceCommandType::Delete, deviceId_, "", DevicePersistencePolicy::Immediate}, 0);
    if (!result.ok()) {
        StaticJsonDocument<384> doc;
        doc["success"] = false;
        doc["code"] = errorCodeForDeviceError(result.validation.error);
        doc["error"] = result.validation.message;
        if (result.validation.error == DeviceError::InvalidRelationship && !result.dependentChildDeviceIds.empty()) {
            doc["code"] = "DEPENDENT_DELETE";
            JsonArray ids = doc.createNestedArray("dependent_child_device_ids");
            for (const DeviceId childId : result.dependentChildDeviceIds) {
                ids.add(childId);
            }
        }
        sendJson(400, doc);
        return;
    }

    StaticJsonDocument<256> doc;
    doc["registry_revision"] = registry_.registryRevision();
    doc["pending_persistence"] = result.pendingPersistence;
    renderOk(doc);
#endif
}

void DeviceRegistryController::cmd() {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    const JsonObjectConst input = getDoc()->as<JsonObjectConst>();
    const DeviceId bodyDeviceId = static_cast<DeviceId>(input["device_id"] | 0U);
    if (bodyDeviceId != 0U && bodyDeviceId != deviceId_) {
        renderError(400, "BAD_ARGS", "device_id mismatch");
        return;
    }

    DeviceCommandType commandType = DeviceCommandType::None;
    const char* commandName = input["command"] | "";
    if (!parseCommandType(commandName, commandType)) {
        renderError(400, "BAD_ARGS", "unsupported command");
        return;
    }

    if (commandType == DeviceCommandType::UpdateConfig) {
        const IDeviceApiAdapter* adapter = record_ != nullptr ? adapters_.find(record_->header.typeId) : nullptr;
        if (adapter != nullptr && record_ != nullptr) {
            std::string error;
            DeviceConfigUpdateRequest updateRequest{};
            if (!adapter->parseUpdateConfigRequest(input, *record_, updateRequest, error)) {
                renderError(400, "BAD_ARGS", error.c_str());
                return;
            }
            const DeviceMutationResult result = registry_.updateConfigAndParent(
                deviceId_, updateRequest.configPayload, updateRequest.configVersion, updateRequest.parentFieldsProvided,
                updateRequest.hasParent, updateRequest.parentDeviceId, 0, parsePolicy(input));
            if (!result.ok()) {
                renderError(400, errorCodeForDeviceError(result.validation.error), result.validation.message);
                return;
            }

            StaticJsonDocument<256> doc;
            doc["registry_revision"] = registry_.registryRevision();
            doc["pending_persistence"] = result.pendingPersistence;
            renderOk(doc);
            return;
        }
    }

    std::string payload;
    if (commandType == DeviceCommandType::SetParent) {
        if (input["has_parent"].is<bool>() && !(input["has_parent"].as<bool>())) {
            payload = "parent=0";
        } else {
            const DeviceId parentId = static_cast<DeviceId>(input["parent_device_id"] | 0U);
            payload = std::string("parent=") + std::to_string(parentId);
        }
    } else {
        payload = input["payload"] | "";
    }

    const DeviceMutationResult result = registry_.command(DeviceCommand{commandType, deviceId_, payload.c_str(), parsePolicy(input)}, 0);
    if (!result.ok()) {
        renderError(400, errorCodeForDeviceError(result.validation.error), result.validation.message);
        return;
    }

    StaticJsonDocument<256> doc;
    doc["registry_revision"] = registry_.registryRevision();
    doc["pending_persistence"] = result.pendingPersistence;
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
    doc["registry_revision"] = registry_.registryRevision();
    doc["pending_persistence"] = registry_.hasPendingPersistence();
    renderOk(doc);
#endif
}

void DeviceRegistryController::options() {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    BaseController::options();
#endif
}

} // namespace ewfm
