#include "portal/controllers/TimeController.h"

#include "time/Iso8601.h"
#include "time/NtpManager.h"

#include <cstring>

#if defined(ARDUINO) && !defined(UNIT_TEST)
#include <ESPAsyncWebServer.h>
#include <TimeLib.h>
#endif

namespace ewfm {

TimeController::TimeController(AsyncWebServerRequest* request, const Action action, NtpManager* ntpManager)
    : BaseController(request, action), ntpManager_(ntpManager) {}

#if defined(ARDUINO) && !defined(UNIT_TEST)
void TimeController::registerRoutes(AsyncWebServer& server, NtpManager* ntpManager) {
    server.on("/api/system/time", HTTP_GET,
              [ntpManager](AsyncWebServerRequest* request) { TimeController(request, Action::Show, ntpManager).dispatch(); });
    server.on(
        "/api/system/time", HTTP_POST, [](AsyncWebServerRequest* request) { (void)request; }, nullptr,
        [ntpManager](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            if (!BaseController::appendRequestBody(request, data, len, index, total)) {
                return;
            }
            TimeController(request, Action::Create, ntpManager).dispatch(static_cast<uint8_t*>(request->_tempObject), total);
            BaseController::clearRequestBody(request);
        });
    server.on("/api/system/time", HTTP_OPTIONS,
              [ntpManager](AsyncWebServerRequest* request) { TimeController(request, Action::Options, ntpManager).dispatch(); });

    server.on("/api/system/time/settings", HTTP_GET,
              [ntpManager](AsyncWebServerRequest* request) { TimeController(request, Action::Index, ntpManager).dispatch(); });
    server.on(
        "/api/system/time/settings", HTTP_PUT, [](AsyncWebServerRequest* request) { (void)request; }, nullptr,
        [ntpManager](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            if (!BaseController::appendRequestBody(request, data, len, index, total)) {
                return;
            }
            TimeController(request, Action::Update, ntpManager).dispatch(static_cast<uint8_t*>(request->_tempObject), total);
            BaseController::clearRequestBody(request);
        });
    server.on("/api/system/time/settings", HTTP_OPTIONS,
              [ntpManager](AsyncWebServerRequest* request) { TimeController(request, Action::Options, ntpManager).dispatch(); });
}
#endif

const BaseController::RulesChain* TimeController::beforeChain() {
    // Does not chain to BaseController::beforeChain(): the shared JSON-body rule would require a
    // body for every action, but the GET/OPTIONS routes here carry none. Only Update/Create parse
    // a body, wired explicitly below.
    static constexpr HookRule rules[] = {
        {&BaseController::beforeCorsOptions, ALL},
        {TimeController::parseJsonBody, A(Action::Update) | A(Action::Create)},
    };
    static const RulesChain node{rules, sizeof(rules) / sizeof(rules[0]), nullptr};
    return &node;
}

bool TimeController::parseJsonBody(BaseController& self) {
    return static_cast<TimeController&>(self).parseBody(512);
}

const char* TimeController::errorCodeForTimeError(const ConfigError error) {
    switch (error) {
    case ConfigError::NtpServerTooLong:
        return "NTP_SERVER_TOO_LONG";
    case ConfigError::TimezoneInvalid:
        return "TIMEZONE_INVALID";
    case ConfigError::SyncIntervalInvalid:
        return "SYNC_INTERVAL_INVALID";
    case ConfigError::StorageError:
        return "STORAGE_ERROR";
    default:
        return "BAD_ARGS";
    }
}

void TimeController::index() {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    if (ntpManager_ == nullptr) {
        renderError(500, "INTERNAL", "time service not available");
        return;
    }
    const TimeConfig& config = ntpManager_->config();
    StaticJsonDocument<256> doc;
    doc["enabled"] = config.enabled;
    doc["ntpServer"] = config.ntpServer;
    doc["timezoneId"] = config.timezoneId;
    doc["syncIntervalSeconds"] = config.syncIntervalSeconds;
    renderOk(doc);
#endif
}

void TimeController::show() {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    if (ntpManager_ == nullptr) {
        renderError(500, "INTERNAL", "time service not available");
        return;
    }
    const TimeConfig& config = ntpManager_->config();
    const bool synced = ntpManager_->synced();

    StaticJsonDocument<384> doc;
    doc["enabled"] = config.enabled;
    doc["synced"] = synced;
    doc["waitingForStation"] = ntpManager_->waitingForStation();
    doc["ntpServer"] = config.ntpServer;
    doc["timezoneId"] = config.timezoneId;
    doc["syncIntervalSeconds"] = config.syncIntervalSeconds;
    doc["source"] = timeSourceName(ntpManager_->lastSource());

    if (synced) {
        const uint32_t currentEpochUtc = static_cast<uint32_t>(now());
        doc["currentEpochUtc"] = currentEpochUtc;
        const std::optional<DateTime>& lastSynced = ntpManager_->lastSyncedUtc();
        doc["lastSyncEpochUtc"] = lastSynced.has_value() ? lastSynced->unixtime() : 0;

        const DateTime local = DateTime::current();
        char buf[32];
        formatIso8601(local, buf, sizeof(buf));
        doc["localTimeIso8601"] = buf;
        doc["utcOffsetMinutes"] = local.tzInfo().offsetSeconds() / 60;
        doc["timezoneAbbrev"] = local.tzInfo().name();
    }
    renderOk(doc);
#endif
}

void TimeController::create() {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    if (ntpManager_ == nullptr) {
        renderError(500, "INTERNAL", "time service not available");
        return;
    }
    const JsonObjectConst input = getDoc()->as<JsonObjectConst>();
    if (!input["iso8601"].is<const char*>()) {
        renderError(400, "BAD_ARGS", "iso8601 is required");
        return;
    }
    const char* iso8601 = input["iso8601"].as<const char*>();
    const std::optional<DateTime> parsed = parseIso8601(iso8601, strlen(iso8601));
    if (!parsed.has_value()) {
        renderError(400, "INVALID_TIME", "iso8601 could not be parsed");
        return;
    }

    ntpManager_->setManualTime(parsed->utcUnixtime());
    show();
#endif
}

void TimeController::update() {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    if (ntpManager_ == nullptr) {
        renderError(500, "INTERNAL", "time service not available");
        return;
    }
    const JsonObjectConst input = getDoc()->as<JsonObjectConst>();
    TimeConfig next = ntpManager_->config();
    if (input["enabled"].is<bool>()) {
        next.enabled = input["enabled"].as<bool>();
    }
    if (input["ntpServer"].is<const char*>()) {
        next.ntpServer = input["ntpServer"].as<const char*>();
    }
    if (input["timezoneId"].is<const char*>()) {
        next.timezoneId = input["timezoneId"].as<const char*>();
    }
    if (input["syncIntervalSeconds"].is<uint32_t>()) {
        next.syncIntervalSeconds = input["syncIntervalSeconds"].as<uint32_t>();
    }

    const ValidationResult result = ntpManager_->applySettings(next);
    if (!result.ok()) {
        renderError(400, errorCodeForTimeError(result.error), result.message);
        return;
    }

    index();
#endif
}

void TimeController::options() {
    BaseController::options();
}

} // namespace ewfm
