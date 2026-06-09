#include "portal/routes/DeviceRegistryRouteHandlers.h"

#include "portal/routes/DeviceRegistryRouteResponder.h"

#if defined(ARDUINO) && !defined(UNIT_TEST)
#include <ESPAsyncWebServer.h>
#endif

namespace ewfm {

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
        DeviceRegistryRouteResponder::sendError(request, 400, "BAD_ARGS", result.validation.message);
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

void DeviceRegistryRouteHandlers::handleDelete(AsyncWebServerRequest* request) const {
    DeviceId deviceId{0};
    if (!parser_.parseDeviceId(request, deviceId)) {
        DeviceRegistryRouteResponder::sendError(request, 400, "BAD_PARAMS", "bad_id");
        return;
    }

    const DeviceMutationResult result =
        registry_.command(DeviceCommand{DeviceCommandType::Delete, deviceId, "", DevicePersistencePolicy::Immediate}, 0);
    if (!result.ok()) {
        DeviceRegistryRouteResponder::sendError(request, 400, "BAD_ARGS", result.validation.message);
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
