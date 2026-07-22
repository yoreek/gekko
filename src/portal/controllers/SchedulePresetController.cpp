#include "portal/controllers/SchedulePresetController.h"

#include "devices/analog/scheduled/ScheduledAnalogOutputDeviceConfig.h"

#include <cstdlib>
#include <cstring>

#if defined(ARDUINO) && !defined(UNIT_TEST)
#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#endif

namespace ewfm {

namespace {
constexpr char kPresetPathPrefix[] = "/api/schedulepresets/";

// Parses "/api/schedulepresets/<deviceId>[/<slot>]" into its numeric parts. deviceId 0 is rejected
// (never a valid device id). hasSlot reports whether a slot segment was present.
bool parsePresetPath(const char* url, uint32_t& deviceId, uint8_t& slot, bool& hasSlot) {
    const size_t prefixLen = sizeof(kPresetPathPrefix) - 1U;
    if (url == nullptr || std::strncmp(url, kPresetPathPrefix, prefixLen) != 0) {
        return false;
    }
    const char* cursor = url + prefixLen;
    char* afterId = nullptr;
    const unsigned long id = std::strtoul(cursor, &afterId, 10);
    if (afterId == cursor || id == 0UL) {
        return false;
    }
    deviceId = static_cast<uint32_t>(id);
    hasSlot = false;
    if (*afterId == '/') {
        const char* slotStart = afterId + 1;
        char* afterSlot = nullptr;
        const unsigned long parsedSlot = std::strtoul(slotStart, &afterSlot, 10);
        if (afterSlot == slotStart) {
            return false;
        }
        slot = static_cast<uint8_t>(parsedSlot);
        hasSlot = true;
    }
    return true;
}
} // namespace

SchedulePresetController::SchedulePresetController(AsyncWebServerRequest* request, const Action action, ISchedulePresetStorage* storage)
    : BaseController(request, action), storage_(storage) {}

#if defined(ARDUINO) && !defined(UNIT_TEST)

void SchedulePresetController::registerRoutes(AsyncWebServer& server, ISchedulePresetStorage* storage) {
    server.on(AsyncURIMatcher::prefix(kPresetPathPrefix), HTTP_GET,
              [storage](AsyncWebServerRequest* request) { SchedulePresetController(request, Action::Index, storage).dispatch(); });
    server.on(AsyncURIMatcher::prefix(kPresetPathPrefix), HTTP_DELETE,
              [storage](AsyncWebServerRequest* request) { SchedulePresetController(request, Action::Destroy, storage).dispatch(); });
    server.on(AsyncURIMatcher::prefix(kPresetPathPrefix), HTTP_OPTIONS,
              [storage](AsyncWebServerRequest* request) { SchedulePresetController(request, Action::Options, storage).dispatch(); });
    server.on(
        AsyncURIMatcher::prefix(kPresetPathPrefix), HTTP_PUT, [](AsyncWebServerRequest* request) { (void)request; }, nullptr,
        [storage](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            if (!BaseController::appendRequestBody(request, data, len, index, total)) {
                return;
            }
            SchedulePresetController(request, Action::Create, storage).dispatch(static_cast<uint8_t*>(request->_tempObject), total);
            BaseController::clearRequestBody(request);
        });
}

#endif

void SchedulePresetController::index() {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    if (storage_ == nullptr) {
        renderError(500, "INTERNAL", "schedule preset storage is not available");
        return;
    }
    uint32_t deviceId = 0U;
    uint8_t slot = 0U;
    bool hasSlot = false;
    if (!parsePresetPath(request_->url().c_str(), deviceId, slot, hasSlot)) {
        renderError(400, "BAD_ARGS", "deviceId is required");
        return;
    }

    DynamicJsonDocument* doc = createDoc(4096);
    if (doc == nullptr) {
        renderError(500, "INTERNAL", "doc allocation failed");
        return;
    }
    (*doc)["deviceId"] = deviceId;
    JsonArray presets = doc->createNestedArray("presets");
    SchedulePresetRecordV1 record{};
    for (uint8_t index = 0U; index < kMaxSchedulePresets; ++index) {
        JsonObject entry = presets.createNestedObject();
        entry["slot"] = index;
        if (storage_->load(deviceId, index, record)) {
            entry["filled"] = true;
            entry["name"] = record.name;
            writeAnalogSchedulePoints(entry.createNestedArray("points"), record.points);
        } else {
            entry["filled"] = false;
        }
    }
    renderOk(*doc);
#endif
}

void SchedulePresetController::create() {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    if (storage_ == nullptr) {
        renderError(500, "INTERNAL", "schedule preset storage is not available");
        return;
    }
    uint32_t deviceId = 0U;
    uint8_t slot = 0U;
    bool hasSlot = false;
    if (!parsePresetPath(request_->url().c_str(), deviceId, slot, hasSlot) || !hasSlot) {
        renderError(400, "BAD_ARGS", "deviceId and slot are required");
        return;
    }
    if (slot >= kMaxSchedulePresets) {
        renderError(400, "BAD_ARGS", "slot is out of range");
        return;
    }

    const JsonObjectConst input = getDoc()->as<JsonObjectConst>();
    const char* name = input["name"] | "";
    if (name[0] == '\0' || std::strlen(name) > kMaxSchedulePresetNameLength) {
        renderError(400, "BAD_ARGS", "name is required and must be 1-32 chars");
        return;
    }
    const JsonVariantConst pointsInput = input["points"];
    if (!pointsInput.is<JsonArrayConst>()) {
        renderError(400, "BAD_ARGS", "points must be an array");
        return;
    }

    ScheduledAnalogOutputPointV1 points[kMaxScheduledAnalogOutputPoints]{};
    for (ScheduledAnalogOutputPointV1& point : points) {
        point.deleted = 1U;
    }
    const char* error = nullptr;
    if (!parseAnalogSchedulePoints(pointsInput.as<JsonArrayConst>(), points, error)) {
        renderError(400, "BAD_ARGS", error);
        return;
    }
    const DeviceValidationResult validation = validateAnalogSchedulePoints(points);
    if (!validation.ok()) {
        renderError(400, "BAD_ARGS", validation.message);
        return;
    }

    // Copy the name out before buildSchedulePresetRecord, which shares the parse doc buffer.
    char nameCopy[kMaxSchedulePresetNameLength + 1]{};
    std::strncpy(nameCopy, name, kMaxSchedulePresetNameLength);

    SchedulePresetRecordV1 record{};
    buildSchedulePresetRecord(record, nameCopy, points);
    if (!storage_->save(deviceId, slot, record)) {
        renderError(500, "STORAGE", "failed to save preset");
        return;
    }
    renderOk();
#endif
}

void SchedulePresetController::destroy() {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    if (storage_ == nullptr) {
        renderError(500, "INTERNAL", "schedule preset storage is not available");
        return;
    }
    uint32_t deviceId = 0U;
    uint8_t slot = 0U;
    bool hasSlot = false;
    if (!parsePresetPath(request_->url().c_str(), deviceId, slot, hasSlot) || !hasSlot) {
        renderError(400, "BAD_ARGS", "deviceId and slot are required");
        return;
    }
    if (slot >= kMaxSchedulePresets) {
        renderError(400, "BAD_ARGS", "slot is out of range");
        return;
    }
    if (!storage_->erase(deviceId, slot)) {
        renderError(500, "STORAGE", "failed to delete preset");
        return;
    }
    renderOk();
#endif
}

void SchedulePresetController::options() {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    BaseController::options();
#endif
}

} // namespace ewfm
