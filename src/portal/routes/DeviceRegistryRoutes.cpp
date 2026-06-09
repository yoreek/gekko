#include "portal/routes/DeviceRegistryRoutes.h"

#if defined(ARDUINO) && !defined(UNIT_TEST)
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <cstdlib>
#endif

namespace ewfm {

DeviceRegistryRoutes::DeviceRegistryRoutes(DeviceRegistry& registry)
    : registry_(registry), adapters_(DeviceApiAdapterRegistry::withDefaults()) {}

#if defined(ARDUINO) && !defined(UNIT_TEST)
void DeviceRegistryRoutes::registerRoutes(AsyncWebServer& server) {
    server.on(AsyncURIMatcher::exact("/api/devices"), HTTP_GET, [this](AsyncWebServerRequest* request) { handleList(request); });
    server.on(AsyncURIMatcher::exact("/api/devices"), HTTP_POST,
              [this](AsyncWebServerRequest* request, JsonVariant& json) { handleCreate(request, json); });
    server.on(AsyncURIMatcher::exact("/api/devices"), HTTP_OPTIONS, [this](AsyncWebServerRequest* request) { handleOptions(request); });
    server.on(AsyncURIMatcher::exact("/api/devices/flush"), HTTP_POST, [this](AsyncWebServerRequest* request) { handleFlush(request); });
    server.on(AsyncURIMatcher::prefix("/api/devices/"), HTTP_GET, [this](AsyncWebServerRequest* request) { handleShow(request); });
    server.on(AsyncURIMatcher::prefix("/api/devices/"), HTTP_DELETE, [this](AsyncWebServerRequest* request) { handleDelete(request); });
    server.on(AsyncURIMatcher::prefix("/api/devices/"), HTTP_OPTIONS, [this](AsyncWebServerRequest* request) { handleOptions(request); });
}

void DeviceRegistryRoutes::handleList(AsyncWebServerRequest* request) {
    AsyncResponseStream* response = request->beginResponseStream("application/json");
    response->print("{\"success\":true,");
    response->printf("\"registry_revision\":%lu,", static_cast<unsigned long>(registry_.registryRevision()));
    response->printf("\"pending_persistence\":%s,", registry_.hasPendingPersistence() ? "true" : "false");
    response->print("\"devices\":[");
    bool first = true;
    for (const auto& record : registry_.list()) {
        const IDeviceApiAdapter* adapter = findAdapterForRecord(record);
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
    addCorsHeaders(response);
    addNoCacheHeaders(response);
    request->send(response);
}

void DeviceRegistryRoutes::handleShow(AsyncWebServerRequest* request) {
    const DeviceRecord* record = findRecord(request);
    if (record == nullptr) {
        sendError(request, 404, "NOT_FOUND", "device not found");
        return;
    }

    DynamicJsonDocument doc(1024);
    doc["success"] = true;
    doc["registry_revision"] = registry_.registryRevision();
    doc["pending_persistence"] = registry_.hasPendingPersistence();
    JsonObject device = doc.createNestedObject("device");
    const IDeviceApiAdapter* adapter = findAdapterForRecord(*record);
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
    sendJson(request, 200, doc);
}

void DeviceRegistryRoutes::handleCreate(AsyncWebServerRequest* request, JsonVariant& json) {
    if (!json.is<JsonObject>()) {
        sendError(request, 400, "BAD_JSON", "bad device payload");
        return;
    }

    std::string error;
    const IDeviceApiAdapter* adapter = findAdapterForCreate(json, error);
    if (adapter == nullptr) {
        sendError(request, 400, "BAD_ARGS", error.c_str());
        return;
    }

    DeviceCreateRequest createRequest;
    if (!adapter->parseCreateRequest(json.as<JsonObjectConst>(), createRequest, error)) {
        sendError(request, 400, "BAD_ARGS", error.c_str());
        return;
    }

    const DeviceCreateResult result = registry_.create(createRequest, 0);
    if (!result.ok()) {
        sendError(request, 400, "BAD_ARGS", result.validation.message);
        return;
    }

    const DeviceRecord* record = registry_.find(result.deviceId);
    if (record == nullptr) {
        sendError(request, 500, "INTERNAL", "created device not found");
        return;
    }

    DynamicJsonDocument doc(1024);
    doc["success"] = true;
    doc["registry_revision"] = registry_.registryRevision();
    doc["pending_persistence"] = result.pendingPersistence;
    JsonObject device = doc.createNestedObject("device");
    adapter->writeDeviceJson(*record, device);
    sendJson(request, 201, doc);
}

void DeviceRegistryRoutes::handleDelete(AsyncWebServerRequest* request) {
    DeviceId deviceId{0};
    if (!parseDeviceId(request, deviceId)) {
        sendError(request, 400, "BAD_PARAMS", "bad_id");
        return;
    }

    const DeviceMutationResult result = registry_.remove(deviceId, 0, DevicePersistencePolicy::Immediate);
    if (!result.ok()) {
        sendError(request, 400, "BAD_ARGS", result.validation.message);
        return;
    }

    DynamicJsonDocument doc(256);
    doc["success"] = true;
    doc["registry_revision"] = registry_.registryRevision();
    doc["pending_persistence"] = result.pendingPersistence;
    sendJson(request, 200, doc);
}

void DeviceRegistryRoutes::handleFlush(AsyncWebServerRequest* request) {
    const DeviceValidationResult result = registry_.flushNow();
    if (!result.ok()) {
        sendError(request, 500, "STORAGE_ERROR", result.message);
        return;
    }

    DynamicJsonDocument doc(256);
    doc["success"] = true;
    doc["registry_revision"] = registry_.registryRevision();
    doc["pending_persistence"] = registry_.hasPendingPersistence();
    sendJson(request, 200, doc);
}

void DeviceRegistryRoutes::handleOptions(AsyncWebServerRequest* request) {
    AsyncWebServerResponse* response = request->beginResponse(204);
    addCorsHeaders(response);
    addNoCacheHeaders(response);
    request->send(response);
}

bool DeviceRegistryRoutes::parseDeviceId(const AsyncWebServerRequest* request, DeviceId& deviceId) const {
    if (request == nullptr) {
        return false;
    }
    const String url = request->url();
    constexpr size_t kPrefixLen = sizeof("/api/devices/") - 1;
    if (url.length() <= kPrefixLen) {
        return false;
    }

    const char* value = url.c_str() + kPrefixLen;
    char* end = nullptr;
    const unsigned long parsed = strtoul(value, &end, 10);
    if (end == value || parsed == 0UL || (end != nullptr && *end != '\0')) {
        return false;
    }
    deviceId = static_cast<DeviceId>(parsed);
    return true;
}

const DeviceRecord* DeviceRegistryRoutes::findRecord(const AsyncWebServerRequest* request) const {
    DeviceId deviceId{0};
    if (!parseDeviceId(request, deviceId)) {
        return nullptr;
    }
    return registry_.find(deviceId);
}

const IDeviceApiAdapter* DeviceRegistryRoutes::findAdapterForRecord(const DeviceRecord& record) const {
    return adapters_.find(record.header.typeId);
}

const IDeviceApiAdapter* DeviceRegistryRoutes::findAdapterForCreate(const JsonVariant& json, std::string& error) const {
    if (json.isNull() || !json.is<JsonObject>()) {
        error = "device payload is missing";
        return nullptr;
    }

    const JsonObjectConst object = json.as<JsonObjectConst>();
    const uint32_t typeId = object["type_id"] | 0U;
    if (typeId != 0U) {
        const IDeviceApiAdapter* adapter = adapters_.find(static_cast<DeviceTypeId>(typeId));
        if (adapter == nullptr) {
            error = "unsupported device type";
        }
        return adapter;
    }

    const char* typeName = object["type"] | "";
    const IDeviceApiAdapter* adapter = adapters_.findByName(typeName);
    if (adapter == nullptr) {
        error = "unsupported device type";
    }
    return adapter;
}

void DeviceRegistryRoutes::addCorsHeaders(AsyncWebServerResponse* response) {
    if (response == nullptr) {
        return;
    }
    response->addHeader("Access-Control-Allow-Origin", "*");
    response->addHeader("Access-Control-Allow-Methods", "GET, POST, DELETE, OPTIONS");
    response->addHeader("Access-Control-Allow-Headers", "Content-Type");
    response->addHeader("Access-Control-Max-Age", "3600");
}

void DeviceRegistryRoutes::addNoCacheHeaders(AsyncWebServerResponse* response) {
    if (response == nullptr) {
        return;
    }
    response->addHeader("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
    response->addHeader("Pragma", "no-cache");
    response->addHeader("Expires", "0");
}

void DeviceRegistryRoutes::sendJson(AsyncWebServerRequest* request, int httpCode, JsonDocument& doc) {
    String payload;
    serializeJson(doc, payload);
    AsyncWebServerResponse* response = request->beginResponse(httpCode, "application/json", payload);
    addCorsHeaders(response);
    addNoCacheHeaders(response);
    request->send(response);
}

void DeviceRegistryRoutes::sendError(AsyncWebServerRequest* request, int httpCode, const char* code, const char* message) {
    DynamicJsonDocument doc(256);
    doc["success"] = false;
    doc["code"] = code;
    doc["error"] = message;
    sendJson(request, httpCode, doc);
}
#endif

} // namespace ewfm
