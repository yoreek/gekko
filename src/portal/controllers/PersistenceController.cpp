#include "portal/controllers/PersistenceController.h"

#include "config/ConfigStore.h"
#include "devices/registry/DeviceRegistry.h"

#if defined(ARDUINO) && !defined(UNIT_TEST)
#include <ESPAsyncWebServer.h>
#endif

namespace ewfm {

PersistenceController::PersistenceController(AsyncWebServerRequest* request, const Action action, ConfigStore* configStore,
                                             DeviceRegistry* deviceRegistry)
    : BaseController(request, action), configStore_(configStore), deviceRegistry_(deviceRegistry) {}

#if defined(ARDUINO) && !defined(UNIT_TEST)
void PersistenceController::registerRoutes(AsyncWebServer& server, ConfigStore* configStore, DeviceRegistry* deviceRegistry) {
    server.on("/api/system/persistence/settings", HTTP_GET, [configStore, deviceRegistry](AsyncWebServerRequest* request) {
        PersistenceController(request, Action::Index, configStore, deviceRegistry).dispatch();
    });
    server.on(
        "/api/system/persistence/settings", HTTP_PUT, [](AsyncWebServerRequest* request) { (void)request; }, nullptr,
        [configStore, deviceRegistry](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            if (!BaseController::appendRequestBody(request, data, len, index, total)) {
                return;
            }
            PersistenceController(request, Action::Update, configStore, deviceRegistry)
                .dispatch(static_cast<uint8_t*>(request->_tempObject), total);
            BaseController::clearRequestBody(request);
        });
    server.on("/api/system/persistence/settings", HTTP_OPTIONS, [configStore, deviceRegistry](AsyncWebServerRequest* request) {
        PersistenceController(request, Action::Options, configStore, deviceRegistry).dispatch();
    });
}
#endif

const BaseController::RulesChain* PersistenceController::beforeChain() {
    // Does not chain to BaseController::beforeChain(): the shared JSON-body rule would require a
    // body for every action, but the GET/OPTIONS routes here carry none.
    static constexpr HookRule rules[] = {
        {&BaseController::beforeCorsOptions, ALL},
        {PersistenceController::parseJsonBody, A(Action::Update)},
    };
    static const RulesChain node{rules, sizeof(rules) / sizeof(rules[0]), nullptr};
    return &node;
}

bool PersistenceController::parseJsonBody(BaseController& self) {
    return static_cast<PersistenceController&>(self).parseBody(256);
}

const char* PersistenceController::errorCodeForPersistenceError(const ConfigError error) {
    switch (error) {
    case ConfigError::PersistenceDebounceInvalid:
        return "PERSISTENCE_DEBOUNCE_INVALID";
    case ConfigError::PersistenceMaxDelayInvalid:
        return "PERSISTENCE_MAX_DELAY_INVALID";
    case ConfigError::PersistenceDebounceExceedsMaxDelay:
        return "PERSISTENCE_DEBOUNCE_EXCEEDS_MAX_DELAY";
    case ConfigError::StorageError:
        return "STORAGE_ERROR";
    default:
        return "BAD_ARGS";
    }
}

void PersistenceController::index() {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    if (configStore_ == nullptr) {
        renderError(500, "INTERNAL", "config store not available");
        return;
    }
    const PersistenceConfig& persistence = configStore_->config().persistence;
    StaticJsonDocument<128> doc;
    doc["debounceMs"] = persistence.debounceMs;
    doc["maxDelayMs"] = persistence.maxDelayMs;
    renderOk(doc);
#endif
}

void PersistenceController::update() {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    if (configStore_ == nullptr) {
        renderError(500, "INTERNAL", "config store not available");
        return;
    }
    const JsonObjectConst input = getDoc()->as<JsonObjectConst>();
    PersistenceConfig next = configStore_->config().persistence;
    if (input["debounceMs"].is<uint32_t>()) {
        next.debounceMs = input["debounceMs"].as<uint32_t>();
    }
    if (input["maxDelayMs"].is<uint32_t>()) {
        next.maxDelayMs = input["maxDelayMs"].as<uint32_t>();
    }

    const ValidationResult result = configStore_->savePersistenceConfig(next);
    if (!result.ok()) {
        renderError(400, errorCodeForPersistenceError(result.error), result.message);
        return;
    }
    if (deviceRegistry_ != nullptr) {
        deviceRegistry_->setPersistenceDelays(next.debounceMs, next.maxDelayMs);
    }

    index();
#endif
}

void PersistenceController::options() {
    BaseController::options();
}

} // namespace ewfm
