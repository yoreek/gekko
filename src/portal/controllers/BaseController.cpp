#include "portal/controllers/BaseController.h"

#include <cstring>

namespace ewfm {

BaseController::BaseController(AsyncWebServerRequest* request, const Action action) : request_(request), action_(action) {}

BaseController::~BaseController() {}

void BaseController::dispatch() {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    if (request_ != nullptr && request_->method() == HTTP_OPTIONS) {
        options();
        return;
    }
#endif

    switch (action_) {
    case Action::Index:
        index();
        break;
    case Action::Show:
        show();
        break;
    case Action::Create:
        create();
        break;
    case Action::Update:
        update();
        break;
    case Action::Destroy:
        destroy();
        break;
    case Action::Options:
        options();
        break;
    case Action::Cmd:
        cmd();
        break;
    case Action::Flush:
        flush();
        break;
    }
}

void BaseController::dispatch(uint8_t* body, const size_t len) {
    body_ = body;
    bodyLen_ = len;
    dispatch();
}

void BaseController::index() {
    renderError(405, "METHOD_NOT_ALLOWED", "Index not implemented");
}

void BaseController::show() {
    renderError(405, "METHOD_NOT_ALLOWED", "Show not implemented");
}

void BaseController::create() {
    renderError(405, "METHOD_NOT_ALLOWED", "Create not implemented");
}

void BaseController::update() {
    renderError(405, "METHOD_NOT_ALLOWED", "Update not implemented");
}

void BaseController::destroy() {
    renderError(405, "METHOD_NOT_ALLOWED", "Destroy not implemented");
}

void BaseController::options() {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    if (request_ == nullptr) {
        return;
    }
    AsyncWebServerResponse* response = request_->beginResponse(204);
    send(response);
#endif
}

void BaseController::cmd() {
    renderError(405, "METHOD_NOT_ALLOWED", "Cmd not implemented");
}

void BaseController::flush() {
    renderError(405, "METHOD_NOT_ALLOWED", "Flush not implemented");
}

bool BaseController::parseBody(const size_t size) {
    if (body_ == nullptr || bodyLen_ == 0) {
        renderError(400, "INVALID", "invalid body");
        return false;
    }

    if (size > 1024U) {
        renderError(400, "INVALID", "invalid body");
        return false;
    }

    StaticJsonDocument<1024> json;
    const DeserializationError error = deserializeJson(json, body_, bodyLen_);
    if (error) {
        renderError(400, "BAD_JSON", "bad json");
        return false;
    }
    return true;
}

void BaseController::renderError(const int httpCode, const char* errCode, const char* message) {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    if (request_ == nullptr) {
        return;
    }

    StaticJsonDocument<256> json;
    addErrorEnvelope(json, errCode, message);
    sendJson(httpCode, json);
#else
    (void)httpCode;
    (void)errCode;
    (void)message;
#endif
}

void BaseController::renderOk(JsonDocument& doc) {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    if (request_ == nullptr) {
        return;
    }

    addSuccessEnvelope(doc);
    doc["status"] = "ok";
    AsyncResponseStream* stream = request_->beginResponseStream("application/json");
    serializeJson(doc, *stream);
    send(stream);
#else
    (void)doc;
#endif
}

void BaseController::renderOk() {
    StaticJsonDocument<64> json;
    renderOk(json);
}

void BaseController::sendJson(const int httpCode, JsonDocument& doc) {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    if (request_ == nullptr) {
        return;
    }

    if (!doc.containsKey("success")) {
        addSuccessEnvelope(doc);
    }
    String payload;
    serializeJson(doc, payload);
    AsyncWebServerResponse* response = request_->beginResponse(httpCode, "application/json", payload);
    send(response);
#else
    (void)httpCode;
    (void)doc;
#endif
}

void BaseController::send(AsyncWebServerResponse* response) {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    applyHeaders(response);
    if (request_ != nullptr) {
        request_->send(response);
    }
#else
    (void)response;
#endif
}

void BaseController::send(AsyncResponseStream* stream) {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    applyHeaders(stream);
    if (request_ != nullptr) {
        request_->send(stream);
    }
#else
    (void)stream;
#endif
}

void BaseController::addCorsHeaders(AsyncWebServerResponse* response) {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    if (response == nullptr) {
        return;
    }
    response->addHeader("Access-Control-Allow-Origin", "*");
    response->addHeader("Access-Control-Allow-Methods", corsAllowMethods());
    response->addHeader("Access-Control-Allow-Headers", corsAllowHeaders());
    response->addHeader("Access-Control-Max-Age", "3600");
#else
    (void)response;
#endif
}

void BaseController::addCorsHeaders(AsyncResponseStream* stream) {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    if (stream == nullptr) {
        return;
    }
    stream->addHeader("Access-Control-Allow-Origin", "*");
    stream->addHeader("Access-Control-Allow-Methods", corsAllowMethods());
    stream->addHeader("Access-Control-Allow-Headers", corsAllowHeaders());
    stream->addHeader("Access-Control-Max-Age", "3600");
#else
    (void)stream;
#endif
}

const char* BaseController::corsAllowMethods() {
    return "GET, POST, PUT, PATCH, DELETE, OPTIONS";
}

const char* BaseController::corsAllowHeaders() {
    return "Content-Type";
}

void BaseController::addSuccessEnvelope(JsonDocument& doc) {
    doc["success"] = true;
}

void BaseController::addErrorEnvelope(JsonDocument& doc, const char* errCode, const char* message) {
    doc["success"] = false;
    doc["code"] = errCode;
    doc["error"] = message;
}

void BaseController::addNoCacheHeaders(AsyncWebServerResponse* response) {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    if (response == nullptr) {
        return;
    }
    response->addHeader("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
    response->addHeader("Pragma", "no-cache");
    response->addHeader("Expires", "0");
#else
    (void)response;
#endif
}

void BaseController::addNoCacheHeaders(AsyncResponseStream* stream) {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    if (stream == nullptr) {
        return;
    }
    stream->addHeader("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
    stream->addHeader("Pragma", "no-cache");
    stream->addHeader("Expires", "0");
#else
    (void)stream;
#endif
}

void BaseController::applyHeaders(AsyncWebServerResponse* response) const {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    if (response == nullptr) {
        return;
    }
    addCorsHeaders(response);
    addNoCacheHeaders(response);
#else
    (void)response;
#endif
}

void BaseController::applyHeaders(AsyncResponseStream* stream) const {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    if (stream == nullptr) {
        return;
    }
    addCorsHeaders(stream);
    addNoCacheHeaders(stream);
#else
    (void)stream;
#endif
}

} // namespace ewfm
