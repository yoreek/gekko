#include "portal/controllers/SystemController.h"

#include "debug/Debug.h"
#include "devices/registry/DeviceRegistry.h"
#include "generated/Version.h"
#include "portal/SystemStatusFormat.h"
#include "portal/controllers/SystemRestartController.h"

#if defined(ARDUINO) && !defined(UNIT_TEST)
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <WiFi.h>
#include <cstdlib>
#include <cstring>
#include <esp_littlefs.h>
#include <esp_partition.h>
#include <esp_system.h>
#include <esp_timer.h>
#include <nvs.h>
#endif

namespace ewfm {

#if defined(ARDUINO) && !defined(UNIT_TEST)

void SystemController::registerRoutes(AsyncWebServer& server, DeviceRegistry* registry) {
    server.on("/api/system/version", HTTP_GET,
              [registry](AsyncWebServerRequest* request) { SystemController(request, Action::Show, registry).dispatch(); });
    server.on("/api/system/version", HTTP_OPTIONS,
              [registry](AsyncWebServerRequest* request) { SystemController(request, Action::Options, registry).dispatch(); });

    server.on("/api/system/status", HTTP_GET,
              [registry](AsyncWebServerRequest* request) { SystemController(request, Action::Index, registry).dispatch(); });
    server.on("/api/system/status", HTTP_OPTIONS,
              [registry](AsyncWebServerRequest* request) { SystemController(request, Action::Options, registry).dispatch(); });

#if defined(WITH_SYSTEM_RESTART_API)
    server.on(
        "/api/system/restart", HTTP_POST, [registry](AsyncWebServerRequest* request) { (void)request; }, nullptr,
        [registry](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            if (!BaseController::appendRequestBody(request, data, len, index, total)) {
                return;
            }

            SystemController(request, Action::Create, registry).dispatch(static_cast<uint8_t*>(request->_tempObject), total);
            BaseController::clearRequestBody(request);
        });
    server.on("/api/system/restart", HTTP_OPTIONS,
              [registry](AsyncWebServerRequest* request) { SystemController(request, Action::Options, registry).dispatch(); });
#else
    (void)server;
    (void)registry;
#endif
}
#endif

SystemController::SystemController(AsyncWebServerRequest* request, const Action action, DeviceRegistry* registry)
    : BaseController(request, action), deviceRegistry_(registry) {}

void SystemController::show() {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    StaticJsonDocument<128> doc;
    doc["version"] = EWFM_FIRMWARE_VERSION;
    doc["buildDate"] = EWFM_FIRMWARE_BUILD_DATE;
    renderOk(doc);
#endif
}

void SystemController::index() {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    auto* doc = createDoc(3072);
    if (doc == nullptr || doc->capacity() == 0U) {
        renderError(500, "STORAGE_ERROR", "out of memory");
        return;
    }

    JsonObject chip = doc->createNestedObject("chip");
    chip["model"] = ESP.getChipModel();
    chip["revision"] = ESP.getChipRevision();
    chip["cores"] = ESP.getChipCores();
    chip["cpuFreqMhz"] = ESP.getCpuFreqMHz();
    chip["flashSizeBytes"] = ESP.getFlashChipSize();

    (*doc)["uptimeSeconds"] = static_cast<uint64_t>(esp_timer_get_time() / 1000000LL);
    (*doc)["resetReason"] = resetReasonToString(static_cast<int>(esp_reset_reason()));

    JsonObject heap = doc->createNestedObject("heap");
    heap["totalBytes"] = ESP.getHeapSize();
    heap["freeBytes"] = ESP.getFreeHeap();
    heap["minFreeBytes"] = ESP.getMinFreeHeap();
    heap["maxAllocBytes"] = ESP.getMaxAllocHeap();

    uint32_t appPartitionBytes = 0;
    JsonArray partitions = doc->createNestedArray("partitions");
    esp_partition_iterator_t it = esp_partition_find(ESP_PARTITION_TYPE_ANY, ESP_PARTITION_SUBTYPE_ANY, nullptr);
    while (it != nullptr) {
        const esp_partition_t* partition = esp_partition_get(it);
        JsonObject entry = partitions.createNestedObject();
        entry["label"] = partition->label;
        entry["type"] = partitionTypeToString(static_cast<int>(partition->type));
        entry["subtype"] = partitionSubtypeToString(static_cast<int>(partition->type), static_cast<int>(partition->subtype));
        entry["offset"] = partition->address;
        entry["sizeBytes"] = partition->size;
        if (partition->type == ESP_PARTITION_TYPE_APP && appPartitionBytes == 0U) {
            appPartitionBytes = partition->size;
        }
        it = esp_partition_next(it);
    }
    esp_partition_iterator_release(it);

    JsonObject sketch = doc->createNestedObject("sketch");
    sketch["usedBytes"] = ESP.getSketchSize();
    sketch["partitionBytes"] = appPartitionBytes;

    static constexpr const char* kFsLabels[] = {"littlefs", "devdata"};
    JsonArray filesystems = doc->createNestedArray("filesystems");
    for (const char* label : kFsLabels) {
        size_t totalBytes = 0;
        size_t usedBytes = 0;
        const bool mounted = esp_littlefs_info(label, &totalBytes, &usedBytes) == ESP_OK;
        JsonObject entry = filesystems.createNestedObject();
        entry["label"] = label;
        entry["mounted"] = mounted;
        entry["totalBytes"] = mounted ? static_cast<uint32_t>(totalBytes) : 0U;
        entry["usedBytes"] = mounted ? static_cast<uint32_t>(usedBytes) : 0U;
    }

    nvs_stats_t nvsStats{};
    JsonObject nvs = doc->createNestedObject("nvs");
    if (nvs_get_stats(nullptr, &nvsStats) == ESP_OK) {
        nvs["usedEntries"] = nvsStats.used_entries;
        nvs["freeEntries"] = nvsStats.free_entries;
        nvs["totalEntries"] = nvsStats.total_entries;
        nvs["namespaceCount"] = nvsStats.namespace_count;
    } else {
        nvs["usedEntries"] = 0;
        nvs["freeEntries"] = 0;
        nvs["totalEntries"] = 0;
        nvs["namespaceCount"] = 0;
    }

    renderOk(*doc);
#endif
}

void SystemController::create() {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    DeviceRegistryRestartPrecondition precondition(deviceRegistry_);
    const SystemRestartDecision decision = SystemRestartController::requestRestart(precondition);
    if (!decision.ok()) {
        EWFM_PORTAL_LOG_WARN("system restart rejected: %s", decision.validation.message);
        renderError(500, "STORAGE_ERROR", decision.validation.message);
        return;
    }

    StaticJsonDocument<128> doc;
    doc["success"] = true;
    doc["rebooting"] = true;
    String payload;
    serializeJson(doc, payload);
    AsyncWebServerResponse* response = request_->beginResponse(200, "application/json", payload);
    response->addHeader("Connection", "close");
    send(response);

    EWFM_PORTAL_LOG_INFO("system restart requested via API");
    SystemRestartController::scheduleReboot();
#endif
}

} // namespace ewfm
