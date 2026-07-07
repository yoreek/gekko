#include "portal/controllers/BaseController.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#if defined(ARDUINO) && !defined(UNIT_TEST)
#include <LittleFS.h>
#endif

namespace ewfm {

namespace {
constexpr size_t kMaxControllerJsonBodyBytes = 8192;
// ArduinoJson's DynamicJsonDocument needs headroom beyond the raw body bytes for its
// per-object/array variant slots, on top of the body content itself (mostly zero-copied).
constexpr size_t kJsonParseOverheadBytes = 1024;

BaseController::RequestFile* requestFilesStorage(AsyncWebServerRequest* request, size_t& fileCount) {
#if defined(ARDUINO) || defined(UNIT_TEST)
    if (request == nullptr || request->_tempObject == nullptr) {
        fileCount = 0U;
        return nullptr;
    }
    auto* state = static_cast<BaseController::RequestState*>(request->_tempObject);
    if (state->magic != BaseController::kRequestStateMagic) {
        fileCount = 0U;
        return nullptr;
    }
    fileCount = state->fileCount;
    return state->files;
#else
    (void)request;
    fileCount = 0U;
    return nullptr;
#endif
}

} // namespace

std::string BaseController::uploadTmpDir_ = "/tmp";

BaseController::BaseController(AsyncWebServerRequest* request, const Action action) : request_(request), action_(action) {}

BaseController::~BaseController() {
    cleanupRequestState();
    delete doc_;
}

const BaseController::RulesChain* BaseController::beforeChain() {
    static constexpr HookRule rules[] = {
        {&BaseController::beforeCorsOptions, ALL},
        {[](BaseController& self) { return self.parseBody(self.bodyLen_ + kJsonParseOverheadBytes); }, A(Action::Create) | A(Action::Cmd)},
    };
    static const RulesChain node{rules, std::size(rules), nullptr};
    return &node;
}

const BaseController::RulesChain* BaseController::afterChain() {
    static constexpr HookRule rules[] = {
        {&BaseController::afterDefaultHeaders, ALL},
    };
    static const RulesChain node{rules, std::size(rules), nullptr};
    return &node;
}

void BaseController::dispatch() {
    if (!runBefore()) {
        return;
    }
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
    case Action::COUNT:
        break;
    }
}

void BaseController::dispatch(uint8_t* body, const size_t len) {
    body_ = body;
    bodyLen_ = len;
    dispatch();
}

const BaseController::RequestFile* BaseController::files() const {
#if defined(ARDUINO) || defined(UNIT_TEST)
    size_t ignored = 0;
    return requestFilesStorage(request_, ignored);
#else
    return nullptr;
#endif
}

size_t BaseController::fileCount() const {
#if defined(ARDUINO) || defined(UNIT_TEST)
    size_t count = 0;
    (void)requestFilesStorage(request_, count);
    return count;
#else
    return 0U;
#endif
}

bool BaseController::claimFile(const size_t index) {
#if defined(ARDUINO) || defined(UNIT_TEST)
    size_t count = 0;
    auto* files = requestFilesStorage(request_, count);
    if (files == nullptr || index >= count) {
        return false;
    }
    files[index].claimed = true;
    return true;
#else
    (void)index;
    return false;
#endif
}

bool BaseController::moveFile(const size_t index, const char* destinationPath) {
#if defined(ARDUINO) || defined(UNIT_TEST)
    size_t count = 0;
    auto* files = requestFilesStorage(request_, count);
    if (files == nullptr || index >= count || destinationPath == nullptr) {
        return false;
    }
    RequestFile& file = files[index];
    if (!file.present || file.claimed || file.tmpPath[0] == '\0') {
        return false;
    }
#if defined(ARDUINO) && !defined(UNIT_TEST)
    if (!LittleFS.rename(file.tmpPath, destinationPath)) {
        return false;
    }
#endif
    file.claimed = true;
    std::strncpy(file.tmpPath, destinationPath, sizeof(file.tmpPath) - 1U);
    file.tmpPath[sizeof(file.tmpPath) - 1U] = '\0';
    return true;
#else
    (void)index;
    (void)destinationPath;
    return false;
#endif
}

void BaseController::setUploadTmpDir(const char* path) {
    uploadTmpDir_ = path == nullptr || path[0] == '\0' ? "/tmp" : path;
}

const char* BaseController::uploadTmpDir() {
    return uploadTmpDir_.c_str();
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
    if (request_ == nullptr)
        return;
    send(request_->beginResponse(204));
#endif
}
void BaseController::cmd() {
    renderError(405, "METHOD_NOT_ALLOWED", "Cmd not implemented");
}
void BaseController::flush() {
    renderError(405, "METHOD_NOT_ALLOWED", "Flush not implemented");
}

bool BaseController::runBefore() {
    return _runBefore(beforeChain());
}
void BaseController::runAfter() {
    _runAfter(afterChain());
}

bool BaseController::_runBefore(const RulesChain* chain) {
    if (chain == nullptr)
        return true;
    if (!_runBefore(chain->prev))
        return false;
    for (size_t i = 0; i < chain->size; ++i) {
        const HookRule& rule = chain->rules[i];
        if ((rule.mask & A(action_)) != 0U && !(rule.fn)(*this))
            return false;
    }
    return true;
}

void BaseController::_runAfter(const RulesChain* chain) {
    if (chain == nullptr)
        return;
    _runAfter(chain->prev);
    for (size_t i = 0; i < chain->size; ++i) {
        const HookRule& rule = chain->rules[i];
        if ((rule.mask & A(action_)) != 0U)
            (void)(rule.fn)(*this);
    }
}

bool BaseController::afterDefaultHeaders(BaseController& self) {
    self.applyDefaultHeaders_ = true;
    return true;
}

bool BaseController::parseBody(const size_t size) {
    if (body_ == nullptr || bodyLen_ == 0 || size > kMaxControllerJsonBodyBytes) {
        renderError(400, "INVALID", "invalid body");
        return false;
    }
    auto* json = createDoc(size);
    if (json == nullptr) {
        renderError(500, "INTERNAL", "doc allocation failed");
        return false;
    }
    const DeserializationError error = deserializeJson(*json, body_, bodyLen_);
    if (error) {
        renderError(400, "BAD_JSON", "bad json");
        return false;
    }
    return true;
}

bool BaseController::appendRequestBody(AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
#if defined(ARDUINO) || defined(UNIT_TEST)
    if (request == nullptr)
        return false;
    if (index == 0U) {
        clearRequestBody(request);
        request->_tempObject = calloc(total + 1U, sizeof(uint8_t));
        if (request->_tempObject == nullptr)
            return false;
    }
    auto* buffer = static_cast<uint8_t*>(request->_tempObject);
    if (buffer == nullptr)
        return false;
    std::memcpy(buffer + index, data, len);
    return (index + len) == total;
#else
    (void)request;
    (void)data;
    (void)len;
    (void)index;
    (void)total;
    return false;
#endif
}

void BaseController::clearRequestBody(AsyncWebServerRequest* request) {
#if defined(ARDUINO) || defined(UNIT_TEST)
    if (request == nullptr || request->_tempObject == nullptr)
        return;
    free(request->_tempObject);
    request->_tempObject = nullptr;
#else
    (void)request;
#endif
}

bool BaseController::beginUploadFile(AsyncWebServerRequest* request, const char* originalFilename, const char* contentType,
                                     const size_t totalBytes, size_t& fileIndex) {
#if defined(ARDUINO) || defined(UNIT_TEST)
    if (request == nullptr) {
        return false;
    }
    if (request->_tempObject == nullptr) {
        request->_tempObject = new RequestState();
    } else {
        auto* existingState = static_cast<RequestState*>(request->_tempObject);
        if (existingState->magic != kRequestStateMagic) {
            return false;
        }
    }
    auto* state = static_cast<RequestState*>(request->_tempObject);
    if (state == nullptr || state->fileCount >= kMaxRequestFiles) {
        return false;
    }
    RequestFile& file = state->files[state->fileCount];
    std::snprintf(file.tmpPath, sizeof(file.tmpPath), "%s/upload-%u.bin", uploadTmpDir(), static_cast<unsigned>(state->fileCount + 1U));
    std::strncpy(file.originalFilename, originalFilename == nullptr ? "" : originalFilename, sizeof(file.originalFilename) - 1U);
    std::strncpy(file.contentType, contentType == nullptr ? "" : contentType, sizeof(file.contentType) - 1U);
    file.size = totalBytes;
    file.received = 0U;
    file.present = true;
    file.claimed = false;
    fileIndex = state->fileCount;
    ++state->fileCount;
    return true;
#else
    (void)request;
    (void)originalFilename;
    (void)contentType;
    (void)totalBytes;
    (void)fileIndex;
    return false;
#endif
}

bool BaseController::appendUploadFile(AsyncWebServerRequest* request, const size_t fileIndex, const uint8_t* data, const size_t len,
                                      const size_t index, const size_t total) {
#if defined(ARDUINO) || defined(UNIT_TEST)
    size_t count = 0;
    auto* files = requestFilesStorage(request, count);
    if (files == nullptr || fileIndex >= count)
        return false;
    RequestFile& file = files[fileIndex];
    if (!file.present || file.tmpPath[0] == '\0')
        return false;
#if defined(ARDUINO) && !defined(UNIT_TEST)
    if (index == 0U && LittleFS.exists(file.tmpPath))
        (void)LittleFS.remove(file.tmpPath);
    // create=true: the upload tmp dir (default "/tmp") is never pre-created on the flashed
    // LittleFS image, and FS::open() only auto-mkdir's missing parent directories when this
    // flag is explicitly set - without it, opening a file under a not-yet-existing directory
    // fails outright.
    File out = LittleFS.open(file.tmpPath, index == 0U ? "w" : "a", true);
    if (!out)
        return false;
    const size_t written = out.write(data, len);
    out.close();
    if (written != len || (index + len) > total || (file.size != 0U && file.received + len > file.size)) {
        return false;
    }
#else
    (void)data;
    if ((file.size != 0U && file.received + len > file.size) || (index + len) > total) {
        return false;
    }
#endif
    file.received += len;
    return true;
#else
    (void)request;
    (void)fileIndex;
    (void)data;
    (void)len;
    (void)index;
    (void)total;
    return false;
#endif
}

bool BaseController::finishUploadFile(AsyncWebServerRequest* request, const size_t fileIndex, const bool final) {
#if defined(ARDUINO) || defined(UNIT_TEST)
    size_t count = 0;
    auto* files = requestFilesStorage(request, count);
    if (files == nullptr || fileIndex >= count)
        return false;
    RequestFile& file = files[fileIndex];
    file.present = true;
    if (file.size == 0U) {
        file.size = file.received;
    }
    (void) final;
    return file.tmpPath[0] != '\0' && file.received == file.size;
#else
    (void)request;
    (void)fileIndex;
    (void) final;
    return false;
#endif
}

void BaseController::cleanupUploadFiles() {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    if (request_ == nullptr || request_->_tempObject == nullptr)
        return;
    auto* state = static_cast<RequestState*>(request_->_tempObject);
    if (state->magic != kRequestStateMagic)
        return;
    for (size_t i = 0; i < state->fileCount; ++i) {
        RequestFile& file = state->files[i];
        if (file.present && !file.claimed && file.tmpPath[0] != '\0') {
            (void)LittleFS.remove(file.tmpPath);
        }
    }
#elif defined(UNIT_TEST)
    (void)this;
#endif
}

const BaseController::RequestFile* BaseController::requestFiles(AsyncWebServerRequest* request, size_t& fileCount) {
    return requestFilesStorage(request, fileCount);
}

void BaseController::renderError(const int httpCode, const char* errCode, const char* message) {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    if (request_ == nullptr)
        return;
    auto* json = createDoc(256);
    if (json == nullptr)
        return;
    json->clear();
    addErrorEnvelope(*json, errCode, message);
    sendJson(httpCode, *json);
#else
    (void)httpCode;
    (void)errCode;
    (void)message;
#endif
}

void BaseController::renderOk(JsonDocument& doc) {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    if (request_ == nullptr)
        return;
    applyDefaultHeaders_ = true;
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
    if (request_ == nullptr)
        return;
    if (!doc.containsKey("success"))
        addSuccessEnvelope(doc);
    String payload;
    serializeJson(doc, payload);
    send(request_->beginResponse(httpCode, "application/json", payload));
#else
    (void)httpCode;
    (void)doc;
#endif
}

void BaseController::send(AsyncWebServerResponse* response) {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    if (request_ != nullptr) {
        runAfter();
        cleanupRequestState();
        request_->send(wrap(response));
    }
#else
    (void)response;
#endif
}

void BaseController::send(AsyncResponseStream* stream) {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    if (request_ != nullptr) {
        runAfter();
        cleanupRequestState();
        request_->send(wrap(stream));
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
const char* BaseController::corsAllowMethods() {
    return "GET, POST, PUT, PATCH, DELETE, OPTIONS";
}
const char* BaseController::corsAllowHeaders() {
    return "Content-Type";
}
bool BaseController::beforeCorsOptions(BaseController& self) {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    if (self.request_ == nullptr || self.request_->method() != HTTP_OPTIONS) {
        return true;
    }

    AsyncWebServerResponse* response = self.request_->beginResponse(204);
    addCorsHeaders(response);
    addNoCacheHeaders(response);
    self.request_->send(response);
    return false;
#else
    (void)self;
    return true;
#endif
}
AsyncWebServerResponse* BaseController::wrap(AsyncWebServerResponse* response) const {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    if (response == nullptr) {
        return response;
    }
    if (applyDefaultHeaders_) {
        addCorsHeaders(response);
        addNoCacheHeaders(response);
    }
#endif
    return response;
}
AsyncResponseStream* BaseController::wrap(AsyncResponseStream* stream) const {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    if (stream == nullptr) {
        return stream;
    }
    if (applyDefaultHeaders_) {
        addCorsHeaders(stream);
        addNoCacheHeaders(stream);
    }
#endif
    return stream;
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
DynamicJsonDocument* BaseController::createDoc(const size_t size) {
    if (doc_ == nullptr)
        doc_ = new DynamicJsonDocument(size);
    else
        doc_->clear();
    return doc_;
}
DynamicJsonDocument* BaseController::getDoc() {
    if (doc_ == nullptr)
        doc_ = new DynamicJsonDocument(64);
    return doc_;
}
void BaseController::cleanupRequestState() {
#if defined(ARDUINO) || defined(UNIT_TEST)
    if (request_ == nullptr || request_->_tempObject == nullptr) {
        return;
    }
    void* tempObject = request_->_tempObject;
    if (tempObject == nullptr) {
        return;
    }
    auto* state = static_cast<RequestState*>(tempObject);
    if (state->magic == kRequestStateMagic) {
        cleanupUploadFiles();
        delete state;
    } else {
        free(tempObject);
    }
    request_->_tempObject = nullptr;
#endif
}

} // namespace ewfm
