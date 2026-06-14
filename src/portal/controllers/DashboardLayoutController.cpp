#include "portal/controllers/DashboardLayoutController.h"

#include <cstdlib>
#include <cstring>

#if defined(ARDUINO) && !defined(UNIT_TEST)
#include <ESPAsyncWebServer.h>
#endif

namespace ewfm {

#if defined(ARDUINO) && !defined(UNIT_TEST)
namespace {
void writeJsonString(::AsyncResponseStream& out, const std::string& value) {
    out.print('\"');
    for (const char c : value) {
        switch (c) {
        case '\"':
            out.print("\\\"");
            break;
        case '\\':
            out.print("\\\\");
            break;
        case '\b':
            out.print("\\b");
            break;
        case '\f':
            out.print("\\f");
            break;
        case '\n':
            out.print("\\n");
            break;
        case '\r':
            out.print("\\r");
            break;
        case '\t':
            out.print("\\t");
            break;
        default:
            if (static_cast<unsigned char>(c) < 0x20U) {
                char buffer[7] = {'\\', 'u', '0', '0', '0', '0', '\0'};
                const unsigned char byte = static_cast<unsigned char>(c);
                constexpr char hex[] = "0123456789abcdef";
                buffer[4] = hex[(byte >> 4U) & 0x0FU];
                buffer[5] = hex[byte & 0x0FU];
                out.print(buffer);
            } else {
                out.print(c);
            }
            break;
        }
    }
    out.print('\"');
}

void writeWidgetJson(::AsyncResponseStream& out, const DashboardLayoutWidget& widget) {
    out.print('[');
    out.print(widget.deviceId);
    out.print(',');
    out.print(widget.x);
    out.print(',');
    out.print(widget.y);
    out.print(',');
    out.print(widget.w);
    out.print(',');
    out.print(widget.h);
    out.print(']');
}

void writeLayoutJson(::AsyncResponseStream& out, const DashboardLayoutSnapshot& layout) {
    out.print('{');
    out.print("\"schema_version\":");
    out.print(layout.schemaVersion);
    out.print(",\"active_panel_id\":");
    writeJsonString(out, layout.activePanelId);
    out.print(",\"panels\":[");
    for (size_t panelIndex = 0; panelIndex < layout.panels.size(); ++panelIndex) {
        if (panelIndex > 0U) {
            out.print(',');
        }
        const DashboardPanelLayout& panel = layout.panels[panelIndex];
        out.print('{');
        out.print("\"id\":");
        writeJsonString(out, panel.id);
        out.print(",\"name\":");
        writeJsonString(out, panel.name);
        out.print(",\"order\":");
        out.print(panel.order);
        out.print(",\"widgets\":[");
        for (size_t widgetIndex = 0; widgetIndex < panel.widgets.size(); ++widgetIndex) {
            if (widgetIndex > 0U) {
                out.print(',');
            }
            writeWidgetJson(out, panel.widgets[widgetIndex]);
        }
        out.print("]}");
    }
    out.print("]}");
}

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
    if (request_ == nullptr) {
        return;
    }

    AsyncResponseStream* stream = request_->beginResponseStream("application/json");
    stream->print('{');
    stream->print("\"success\":true,");
    stream->print("\"revision\":");
    stream->print(result.revision);
    stream->print(",\"layout_defaulted\":");
    stream->print(result.defaulted ? "true" : "false");
    stream->print(",\"layout\":");
    writeLayoutJson(*stream, result.layout);
    stream->print('}');
    send(stream);
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

    if (request_ == nullptr) {
        return;
    }

    AsyncWebServerResponse* response = request_->beginResponse(204);
    send(response);
#endif
}

void DashboardLayoutController::options() {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    BaseController::options();
#endif
}

} // namespace ewfm
