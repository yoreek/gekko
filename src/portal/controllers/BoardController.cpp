#include "portal/controllers/BoardController.h"

#include "config/ConfigStore.h"
#include "platform/BoardPinCapabilities.h"

#if defined(ARDUINO) && !defined(UNIT_TEST)
#include <ESPAsyncWebServer.h>
#endif

namespace ewfm {

BoardController::BoardController(AsyncWebServerRequest* request, const Action action, ConfigStore* configStore)
    : BaseController(request, action), configStore_(configStore) {}

#if defined(ARDUINO) && !defined(UNIT_TEST)
void BoardController::registerRoutes(AsyncWebServer& server, ConfigStore* configStore) {
    server.on("/api/system/board", HTTP_GET,
              [configStore](AsyncWebServerRequest* request) { BoardController(request, Action::Index, configStore).dispatch(); });
    server.on(
        "/api/system/board", HTTP_PUT, [](AsyncWebServerRequest* request) { (void)request; }, nullptr,
        [configStore](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            if (!BaseController::appendRequestBody(request, data, len, index, total)) {
                return;
            }
            BoardController(request, Action::Update, configStore).dispatch(static_cast<uint8_t*>(request->_tempObject), total);
            BaseController::clearRequestBody(request);
        });
    server.on("/api/system/board", HTTP_OPTIONS,
              [configStore](AsyncWebServerRequest* request) { BoardController(request, Action::Options, configStore).dispatch(); });
}
#endif

const BaseController::RulesChain* BoardController::beforeChain() {
    // Does not chain to BaseController::beforeChain(): the shared JSON-body rule would require a
    // body for every action, but the GET/OPTIONS route here carries none. Only Update parses one,
    // wired explicitly below (same reasoning as TimeController::beforeChain()).
    static constexpr HookRule rules[] = {
        {&BaseController::beforeCorsOptions, ALL},
        {BoardController::parseJsonBody, A(Action::Update)},
    };
    static const RulesChain node{rules, sizeof(rules) / sizeof(rules[0]), nullptr};
    return &node;
}

bool BoardController::parseJsonBody(BaseController& self) {
    return static_cast<BoardController&>(self).parseBody(256);
}

void BoardController::index() {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    if (configStore_ == nullptr) {
        renderError(500, "INTERNAL", "config store not available");
        return;
    }
    StaticJsonDocument<512> doc;
    doc["chip"] = kChipId;
    doc["selectedBoardId"] = boardModelName(configStore_->config().boardModel);
    JsonArray supported = doc.createNestedArray("supportedBoardIds");
    for (size_t i = 0; i < kSupportedBoardIdCount; ++i) {
        supported.add(kSupportedBoardIds[i]);
    }
    renderOk(doc);
#endif
}

void BoardController::update() {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    if (configStore_ == nullptr) {
        renderError(500, "INTERNAL", "config store not available");
        return;
    }
    const JsonObjectConst input = getDoc()->as<JsonObjectConst>();
    if (!input["boardId"].is<const char*>()) {
        renderError(400, "BAD_ARGS", "boardId is required");
        return;
    }

    BoardModel parsedModel{};
    if (!boardModelFromString(input["boardId"].as<const char*>(), parsedModel)) {
        renderError(400, "BOARD_ID_INVALID", "boardId is not a supported model for this chip");
        return;
    }

    DeviceConfig next = configStore_->config();
    next.boardModel = parsedModel;
    const ValidationResult result = configStore_->save(next);
    if (!result.ok()) {
        renderError(500, "STORAGE_ERROR", result.message);
        return;
    }

    index();
#endif
}

void BoardController::options() {
    BaseController::options();
}

} // namespace ewfm
