#pragma once

#include <ArduinoJson.h>
#include <cstddef>
#include <cstdint>
#include <string>

#if defined(ARDUINO) && !defined(UNIT_TEST)
#include <ESPAsyncWebServer.h>
#elif defined(UNIT_TEST)
#include <ESPAsyncWebServer.h>
#else
class AsyncWebServerRequest;
class AsyncWebServerResponse;
class AsyncResponseStream;
#endif

namespace ewfm {

class BaseController {
public:
    static constexpr uint32_t kRequestStateMagic = 0x57554631u;
    static constexpr size_t kMaxRequestFiles = 3;
    static constexpr size_t kMaxTmpPathLength = 96;
    static constexpr size_t kMaxOriginalFilenameLength = 96;
    static constexpr size_t kMaxContentTypeLength = 64;

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
    struct RequestFile {
        char tmpPath[kMaxTmpPathLength]{};
        char originalFilename[kMaxOriginalFilenameLength]{};
        char contentType[kMaxContentTypeLength]{};
        size_t size{0};
        size_t received{0};
        bool claimed{false};
        bool present{false};
    };
    struct RequestState {
        uint32_t magic{kRequestStateMagic};
        RequestFile files[kMaxRequestFiles]{};
        size_t fileCount{0};
    };
    explicit BaseController(AsyncWebServerRequest* request, Action action);
    virtual ~BaseController();

    void dispatch();
    void dispatch(uint8_t* body, size_t len);

    const RequestFile* files() const;
    size_t fileCount() const;

    bool claimFile(size_t index);
    bool moveFile(size_t index, const char* destinationPath);
    static void setUploadTmpDir(const char* path);
    static const char* uploadTmpDir();
    static bool appendRequestBody(AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total);
    static void clearRequestBody(AsyncWebServerRequest* request);

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
    virtual const RulesChain* afterChain();

    bool parseBody(size_t size = 1024);

    void renderError(int httpCode, const char* errCode, const char* message);
    void renderOk(JsonDocument& doc);
    void renderOk();
    void sendJson(int httpCode, JsonDocument& doc);

    void send(AsyncWebServerResponse* response);
    void send(AsyncResponseStream* stream);

    static bool beginUploadFile(AsyncWebServerRequest* request, const char* originalFilename, const char* contentType, size_t totalBytes,
                                size_t& fileIndex);
    static bool appendUploadFile(AsyncWebServerRequest* request, size_t fileIndex, const uint8_t* data, size_t len, size_t index,
                                 size_t total);
    static bool finishUploadFile(AsyncWebServerRequest* request, size_t fileIndex, bool final);
    void cleanupUploadFiles();
    static const RequestFile* requestFiles(AsyncWebServerRequest* request, size_t& fileCount);

    static const char* corsAllowMethods();
    static const char* corsAllowHeaders();
    static bool beforeCorsOptions(BaseController& self);
    static bool afterDefaultHeaders(BaseController& self);
    static void addSuccessEnvelope(JsonDocument& doc);
    static void addErrorEnvelope(JsonDocument& doc, const char* errCode, const char* message);
    static void addCorsHeaders(AsyncWebServerResponse* response);
    static void addNoCacheHeaders(AsyncWebServerResponse* response);

    DynamicJsonDocument* createDoc(size_t size = 1024);
    DynamicJsonDocument* getDoc();

private:
    bool runBefore();
    void runAfter();
    bool _runBefore(const RulesChain* chain);
    void _runAfter(const RulesChain* chain);
    AsyncWebServerResponse* wrap(AsyncWebServerResponse* response) const;
    AsyncResponseStream* wrap(AsyncResponseStream* stream) const;
    void cleanupRequestState();

    DynamicJsonDocument* doc_{nullptr};
    bool applyDefaultHeaders_{true};
    static std::string uploadTmpDir_;
};

} // namespace ewfm
