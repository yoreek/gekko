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
        COUNT,
    };

    using ActionMask = uint32_t;
    static constexpr ActionMask A(const Action action) {
        return 1u << static_cast<uint8_t>(action);
    }
    static constexpr ActionMask ALL = (1u << static_cast<uint8_t>(Action::COUNT)) - 1u;

    using HookFn = bool (*)(BaseController& self);
    struct HookRule {
        HookFn fn;
        ActionMask mask;
    };
    struct RulesChain {
        const HookRule* rules;
        size_t size;
        const RulesChain* prev{nullptr};
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

    virtual const RulesChain* beforeChain();

    bool parseBody(size_t size = 1024);

    void renderError(int httpCode, const char* errCode, const char* message);
    void renderOk(JsonDocument& doc);
    void renderOk();
    void sendJson(int httpCode, JsonDocument& doc);

    void send(AsyncWebServerResponse* response);
    void send(AsyncResponseStream* stream);

    static const char* corsAllowMethods();
    static const char* corsAllowHeaders();
    static bool beforeCorsOptions(BaseController& self);
    static void addSuccessEnvelope(JsonDocument& doc);
    static void addErrorEnvelope(JsonDocument& doc, const char* errCode, const char* message);
    static void addCorsHeaders(AsyncWebServerResponse* response);
    static void addNoCacheHeaders(AsyncWebServerResponse* response);

private:
    bool runBefore();
    bool _runBefore(const RulesChain* chain);
};

} // namespace ewfm
