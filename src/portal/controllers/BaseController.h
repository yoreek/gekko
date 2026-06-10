#pragma once

#include <ArduinoJson.h>

class AsyncWebServerRequest;
class AsyncWebServerResponse;
class AsyncResponseStream;

#if defined(ARDUINO) && !defined(UNIT_TEST)
#include <ESPAsyncWebServer.h>
#endif

namespace ewfm {

class BaseController {
public:
    enum class Action : uint8_t {
        Index,
        Show,
        Create,
        Update,
        Destroy,
        Options,
        Cmd,
        Flush,
    };

    explicit BaseController(AsyncWebServerRequest* request, Action action);
    virtual ~BaseController();

    void dispatch();
    void dispatch(uint8_t* body, size_t len);

protected:
    AsyncWebServerRequest* request_{nullptr};
    Action action_{Action::Index};
    uint8_t* body_{nullptr};
    size_t bodyLen_{0};

    virtual void index();
    virtual void show();
    virtual void create();
    virtual void update();
    virtual void destroy();
    virtual void options();
    virtual void cmd();
    virtual void flush();

    bool parseBody(size_t size = 1024);

    void renderError(int httpCode, const char* errCode, const char* message);
    void renderOk(JsonDocument& doc);
    void renderOk();
    void sendJson(int httpCode, JsonDocument& doc);

    void send(AsyncWebServerResponse* response);
    void send(AsyncResponseStream* stream);

    static const char* corsAllowMethods();
    static const char* corsAllowHeaders();
    static void addSuccessEnvelope(JsonDocument& doc);
    static void addErrorEnvelope(JsonDocument& doc, const char* errCode, const char* message);
    static void addCorsHeaders(AsyncWebServerResponse* response);
    static void addCorsHeaders(AsyncResponseStream* stream);
    static void addNoCacheHeaders(AsyncWebServerResponse* response);
    static void addNoCacheHeaders(AsyncResponseStream* stream);

private:
    void applyHeaders(AsyncWebServerResponse* response) const;
    void applyHeaders(AsyncResponseStream* stream) const;
};

} // namespace ewfm
