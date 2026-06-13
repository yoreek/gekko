#include "portal/controllers/DashboardLayoutController.h"

#include <cstdlib>
#include <cstring>

#if defined(ARDUINO) && !defined(UNIT_TEST)
#include <ESPAsyncWebServer.h>
#endif

namespace ewfm {

#if defined(ARDUINO) && !defined(UNIT_TEST)
namespace {
bool appendRequestBody(AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
    if (request == nullptr || total > DashboardLayoutStore::kMaxSerializedBytes) {
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

void DashboardLayoutController::registerRoutes(AsyncWebServer& server, DashboardLayoutStore& store) {
    server.on(AsyncURIMatcher::exact("/api/dashboard/layout"), HTTP_GET,
              [&store](AsyncWebServerRequest* request) { DashboardLayoutController(request, Action::Show, store).dispatch(); });
    server.on(
        AsyncURIMatcher::exact("/api/dashboard/layout"), HTTP_PUT, [&store](AsyncWebServerRequest* request) { (void)request; }, nullptr,
        [&store](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            if (!appendRequestBody(request, data, len, index, total)) {
                return;
            }

            DashboardLayoutController(request, Action::Update, store).dispatch(static_cast<uint8_t*>(request->_tempObject), total);
            clearRequestBody(request);
        });
    server.on(AsyncURIMatcher::exact("/api/dashboard/layout"), HTTP_OPTIONS,
              [&store](AsyncWebServerRequest* request) { DashboardLayoutController(request, Action::Options, store).dispatch(); });
}
#endif

const BaseController::RulesChain* DashboardLayoutController::beforeChain() {
    static constexpr HookRule rules[] = {
        {DashboardLayoutController::parseUpdateBody, A(Action::Update)},
    };
    static const RulesChain node{rules, sizeof(rules) / sizeof(rules[0]), BaseController::beforeChain()};
    return &node;
}

bool DashboardLayoutController::parseUpdateBody(BaseController& self) {
    return static_cast<DashboardLayoutController&>(self).parseBody(DashboardLayoutStore::kMaxSerializedBytes);
}

void DashboardLayoutController::show() {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    const DashboardLayoutLoadResult result = store_.load();
    DynamicJsonDocument doc(DashboardLayoutStore::kMaxSerializedBytes);
    doc["revision"] = result.revision;
    doc["layout_defaulted"] = result.defaulted;
    JsonObject layout = doc.createNestedObject("layout");
    store_.writeLayoutJson(layout, result.layout);
    renderOk(doc);
#endif
}

void DashboardLayoutController::update() {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    const JsonVariantConst input = getDoc()->as<JsonVariantConst>();
    const DashboardLayoutSaveResult result = store_.saveJson(input["layout"].isNull() ? input : input["layout"].as<JsonVariantConst>());
    if (!result.ok()) {
        renderError(400, store_.errorCode(result.validation.error), result.validation.message);
        return;
    }

    DynamicJsonDocument doc(DashboardLayoutStore::kMaxSerializedBytes);
    doc["revision"] = result.revision;
    doc["layout_defaulted"] = false;
    JsonObject layout = doc.createNestedObject("layout");
    store_.writeLayoutJson(layout, result.layout);
    renderOk(doc);
#endif
}

void DashboardLayoutController::options() {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    BaseController::options();
#endif
}

} // namespace ewfm
