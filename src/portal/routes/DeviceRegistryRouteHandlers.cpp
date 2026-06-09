#include "portal/routes/DeviceRegistryRouteHandlers.h"

#include "portal/routes/DeviceRegistryRouteResponder.h"

#if defined(ARDUINO) && !defined(UNIT_TEST)
#include <ESPAsyncWebServer.h>
#include <cstring>
#endif

namespace ewfm {

#if defined(ARDUINO) && !defined(UNIT_TEST)
namespace {
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

const char* errorCodeForDeviceError(DeviceError error) {
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

DevicePersistencePolicy parsePolicy(const JsonObjectConst& input) {
    const char* value = input["persistence_policy"] | "immediate";
    if (std::strcmp(value, "delayed") == 0) {
        return DevicePersistencePolicy::Delayed;
    }
    if (std::strcmp(value, "coalesced") == 0) {
        return DevicePersistencePolicy::Coalesced;
    }
    return DevicePersistencePolicy::Immediate;
}

bool parseCommandType(const char* value, DeviceCommandType& type) {
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
} // namespace
#endif

DeviceRegistryRouteHandlers::DeviceRegistryRouteHandlers(DeviceRegistry& registry, const DeviceApiAdapterRegistry& adapters)
    : registry_(registry), parser_(registry, adapters) {}

#if defined(ARDUINO) && !defined(UNIT_TEST)
void DeviceRegistryRouteHandlers::handleList(AsyncWebServerRequest* request) const {
    AsyncResponseStream* response = request->beginResponseStream("application/json");
    response->print("{\"success\":true,");
    response->printf("\"registry_revision\":%lu,", static_cast<unsigned long>(registry_.registryRevision()));
    response->printf("\"pending_persistence\":%s,", registry_.hasPendingPersistence() ? "true" : "false");
    response->print("\"devices\":[");
    bool first = true;
    for (const auto& record : registry_.list()) {
        const IDeviceApiAdapter* adapter = parser_.findAdapterForRecord(record);
        if (!first) {
            response->print(',');
        }
        first = false;
        DynamicJsonDocument item(512);
        JsonObject device = item.to<JsonObject>();
        if (adapter != nullptr) {
            adapter->writeDeviceJson(record, device);
        } else {
            device["device_id"] = record.header.deviceId;
            device["type_id"] = record.header.typeId;
            device["name"] = record.name;
            device["enabled"] = record.enabled;
        }
        const DeviceRecord* persisted = registry_.find(record.header.deviceId);
        const DeviceStatus lifecycleStatus = registry_.runtime(record.header.deviceId) != nullptr
                                                 ? registry_.runtime(record.header.deviceId)->status()
                                                 : (persisted != nullptr ? persisted->status : record.status);
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
    DeviceRegistryRouteResponder::addCorsHeaders(response);
    DeviceRegistryRouteResponder::addNoCacheHeaders(response);
    request->send(response);
}

void DeviceRegistryRouteHandlers::handleShow(AsyncWebServerRequest* request) const {
    const DeviceRecord* record = parser_.findRecord(request);
    if (record == nullptr) {
        DeviceRegistryRouteResponder::sendError(request, 404, "NOT_FOUND", "device not found");
        return;
    }

    DynamicJsonDocument doc(1024);
    doc["success"] = true;
    doc["registry_revision"] = registry_.registryRevision();
    doc["pending_persistence"] = registry_.hasPendingPersistence();
    JsonObject device = doc.createNestedObject("device");
    const IDeviceApiAdapter* adapter = parser_.findAdapterForRecord(*record);
    DeviceRecord effectiveRecord = *record;
    effectiveRecord.status = registry_.effectiveStatus(record->header.deviceId);
    if (adapter != nullptr) {
        adapter->writeDeviceJson(effectiveRecord, device);
    } else {
        device["device_id"] = effectiveRecord.header.deviceId;
        device["type_id"] = effectiveRecord.header.typeId;
        device["name"] = effectiveRecord.name;
        device["enabled"] = effectiveRecord.enabled;
    }
    const DeviceStatus lifecycleStatus = registry_.runtime(effectiveRecord.header.deviceId) != nullptr
                                             ? registry_.runtime(effectiveRecord.header.deviceId)->status()
                                             : record->status;
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
    DeviceRegistryRouteResponder::sendJson(request, 200, doc);
}

void DeviceRegistryRouteHandlers::handleCreate(AsyncWebServerRequest* request, JsonVariant& json) const {
    if (!json.is<JsonObject>()) {
        DeviceRegistryRouteResponder::sendError(request, 400, "BAD_JSON", "bad device payload");
        return;
    }

    std::string error;
    const IDeviceApiAdapter* adapter = parser_.findAdapterForCreate(json, error);
    if (adapter == nullptr) {
        DeviceRegistryRouteResponder::sendError(request, 400, "BAD_ARGS", error.c_str());
        return;
    }

    DeviceCreateRequest createRequest;
    if (!adapter->parseCreateRequest(json.as<JsonObjectConst>(), createRequest, error)) {
        DeviceRegistryRouteResponder::sendError(request, 400, "BAD_ARGS", error.c_str());
        return;
    }

    const DeviceCreateResult result = registry_.command(createRequest, 0);
    if (!result.ok()) {
        DeviceRegistryRouteResponder::sendError(request, 400, errorCodeForDeviceError(result.validation.error), result.validation.message);
        return;
    }

    const DeviceRecord* record = registry_.find(result.deviceId);
    if (record == nullptr) {
        DeviceRegistryRouteResponder::sendError(request, 500, "INTERNAL", "created device not found");
        return;
    }

    DynamicJsonDocument doc(1024);
    doc["success"] = true;
    doc["registry_revision"] = registry_.registryRevision();
    doc["pending_persistence"] = result.pendingPersistence;
    JsonObject device = doc.createNestedObject("device");
    adapter->writeDeviceJson(*record, device);
    DeviceRegistryRouteResponder::sendJson(request, 201, doc);
}

void DeviceRegistryRouteHandlers::handleCommand(AsyncWebServerRequest* request, JsonVariant& json) const {
    if (!json.is<JsonObject>()) {
        DeviceRegistryRouteResponder::sendError(request, 400, "BAD_JSON", "bad command payload");
        return;
    }

    const JsonObjectConst input = json.as<JsonObjectConst>();
    const DeviceId deviceId = static_cast<DeviceId>(input["device_id"] | 0U);
    if (deviceId == 0) {
        DeviceRegistryRouteResponder::sendError(request, 400, "BAD_ARGS", "device_id is required");
        return;
    }

    DeviceCommandType commandType = DeviceCommandType::None;
    const char* commandName = input["command"] | "";
    if (!parseCommandType(commandName, commandType)) {
        DeviceRegistryRouteResponder::sendError(request, 400, "BAD_ARGS", "unsupported command");
        return;
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

    const DeviceMutationResult result = registry_.command(DeviceCommand{commandType, deviceId, payload.c_str(), parsePolicy(input)}, 0);
    if (!result.ok()) {
        DeviceRegistryRouteResponder::sendError(request, 400, errorCodeForDeviceError(result.validation.error), result.validation.message);
        return;
    }

    DynamicJsonDocument doc(256);
    doc["success"] = true;
    doc["registry_revision"] = registry_.registryRevision();
    doc["pending_persistence"] = result.pendingPersistence;
    DeviceRegistryRouteResponder::sendJson(request, 200, doc);
}

void DeviceRegistryRouteHandlers::handleDelete(AsyncWebServerRequest* request) const {
    DeviceId deviceId{0};
    if (!parser_.parseDeviceId(request, deviceId)) {
        DeviceRegistryRouteResponder::sendError(request, 400, "BAD_PARAMS", "bad_id");
        return;
    }

    const DeviceMutationResult result =
        registry_.command(DeviceCommand{DeviceCommandType::Delete, deviceId, "", DevicePersistencePolicy::Immediate}, 0);
    if (!result.ok()) {
        DynamicJsonDocument doc(384);
        doc["success"] = false;
        doc["code"] = errorCodeForDeviceError(result.validation.error);
        doc["error"] = result.validation.message;
        if (result.validation.error == DeviceError::InvalidRelationship && !result.dependentChildDeviceIds.empty()) {
            doc["code"] = "DEPENDENT_DELETE";
            JsonArray ids = doc.createNestedArray("dependent_child_device_ids");
            for (DeviceId childId : result.dependentChildDeviceIds) {
                ids.add(childId);
            }
        }
        DeviceRegistryRouteResponder::sendJson(request, 400, doc);
        return;
    }

    DynamicJsonDocument doc(256);
    doc["success"] = true;
    doc["registry_revision"] = registry_.registryRevision();
    doc["pending_persistence"] = result.pendingPersistence;
    DeviceRegistryRouteResponder::sendJson(request, 200, doc);
}

void DeviceRegistryRouteHandlers::handleFlush(AsyncWebServerRequest* request) const {
    const DeviceValidationResult result = registry_.flushNow();
    if (!result.ok()) {
        DeviceRegistryRouteResponder::sendError(request, 500, "STORAGE_ERROR", result.message);
        return;
    }

    DynamicJsonDocument doc(256);
    doc["success"] = true;
    doc["registry_revision"] = registry_.registryRevision();
    doc["pending_persistence"] = registry_.hasPendingPersistence();
    DeviceRegistryRouteResponder::sendJson(request, 200, doc);
}

void DeviceRegistryRouteHandlers::handleOptions(AsyncWebServerRequest* request) const {
    DeviceRegistryRouteResponder::sendOptions(request);
}
#endif

} // namespace ewfm
