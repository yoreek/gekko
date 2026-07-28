#include "portal/controllers/BlobController.h"

#include "platform/BlobKeyValidation.h"

#include <cstring>

#if defined(ARDUINO) && !defined(UNIT_TEST)
#include <ESPAsyncWebServer.h>
#endif

namespace ewfm {

namespace {
constexpr char kBlobsPrefix[] = "/api/blobs/";
// Generous relative to the 256 KiB devdata partition this store shares with the dose journal and
// schedule presets - bounds any single object so one PUT cannot monopolize the whole partition.
// The free-space guard in LittleFsBlobStore::beginPut() is the actual capacity backstop; this is
// just a sanity ceiling on top of it.
constexpr size_t kBlobControllerMaxPutBytes = 65536;
} // namespace

BlobController::BlobController(AsyncWebServerRequest* request, const Action action, LittleFsBlobStore* store, const PutOutcome putOutcome,
                               std::string generatedKey)
    : BaseController(request, action), store_(store), putOutcome_(putOutcome), generatedKey_(std::move(generatedKey)) {}

bool BlobController::parseBlobKey(const char* url, std::string& key) {
    constexpr size_t kPrefixLen = sizeof(kBlobsPrefix) - 1;
    if (url == nullptr || std::strlen(url) <= kPrefixLen || std::strncmp(url, kBlobsPrefix, kPrefixLen) != 0) {
        return false;
    }
    const char* rawKey = url + kPrefixLen;
    const size_t rawKeyLen = std::strlen(rawKey);
    if (!isValidBlobKey(rawKey, rawKeyLen)) {
        return false;
    }
    key.assign(rawKey, rawKeyLen);
    return true;
}

bool BlobController::requireBlobKey(BaseController& self) {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    auto& ctl = static_cast<BlobController&>(self); // NOLINT
    if (ctl.request_ == nullptr || !parseBlobKey(ctl.request_->url().c_str(), ctl.key_)) {
        ctl.renderError(400, "BAD_PARAMS", "invalid or missing blob key");
        return false;
    }
    return true;
#else
    (void)self;
    return false;
#endif
}

const BaseController::RulesChain* BlobController::beforeChain() {
    // Update doesn't need this hook: the PUT body-handler already parsed+validated the key at
    // index==0 (before any bytes were written) and hands the result in via putOutcome_. The base
    // chain's auto-JSON parseBody hook is scoped only to Create/Cmd, so it never fires for our
    // Show/Update/Destroy actions - no need to break chaining to avoid it.
    static constexpr HookRule rules[] = {
        {&BlobController::requireBlobKey, A(Action::Show) | A(Action::Destroy)},
    };
    static const RulesChain node{rules, sizeof(rules) / sizeof(rules[0]), BaseController::beforeChain()};
    return &node;
}

void BlobController::show() {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    if (store_ == nullptr) {
        renderError(500, "INTERNAL", "blob store is not available");
        return;
    }
    File file = store_->openForRead(key_);
    if (!file) {
        renderError(404, "NOT_FOUND", "blob not found");
        return;
    }
    send(request_->beginResponse(file, String(file.path()), "application/octet-stream"));
#endif
}

void BlobController::destroy() {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    if (store_ == nullptr) {
        renderError(500, "INTERNAL", "blob store is not available");
        return;
    }
    // removeByPrefix unifies single-blob delete and prefix delete: if key_ names a leaf file,
    // only that file goes; if it names a directory-shaped prefix, everything under it goes.
    (void)store_->removeByPrefix(key_);
    renderOk();
#endif
}

void BlobController::update() {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    switch (putOutcome_) {
    case PutOutcome::BadKey:
        renderError(400, "BAD_PARAMS", "invalid or missing blob key");
        return;
    case PutOutcome::TooLarge:
        renderError(413, "TOO_LARGE", "blob exceeds max size");
        return;
    case PutOutcome::WriteFailed:
        renderError(500, "INTERNAL", "blob write failed");
        return;
    case PutOutcome::KeyGenFailed:
        renderError(500, "INTERNAL", "unable to generate a unique blob key");
        return;
    case PutOutcome::Ok:
        break;
    }
    if (!generatedKey_.empty()) {
        StaticJsonDocument<160> doc;
        doc["key"] = generatedKey_;
        renderOk(doc);
        return;
    }
    renderOk();
#endif
}

void BlobController::options() {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    BaseController::options();
#endif
}

#if defined(ARDUINO) && !defined(UNIT_TEST)

namespace {
struct BlobPutState {
    LittleFsBlobStore::WriteHandle handle;
    BlobController::PutOutcome outcome{BlobController::PutOutcome::Ok};
};

struct BlobPostState {
    LittleFsBlobStore::WriteHandle handle;
    std::string key; // set once beginPutGenerated() succeeds in finding a free key
    BlobController::PutOutcome outcome{BlobController::PutOutcome::Ok};
};
} // namespace

void BlobController::registerRoutes(AsyncWebServer& server, LittleFsBlobStore* store) {
    server.on(AsyncURIMatcher::prefix(kBlobsPrefix), HTTP_GET,
              [store](AsyncWebServerRequest* request) { BlobController(request, Action::Show, store).dispatch(); });

    server.on(AsyncURIMatcher::prefix(kBlobsPrefix), HTTP_DELETE,
              [store](AsyncWebServerRequest* request) { BlobController(request, Action::Destroy, store).dispatch(); });

    server.on(AsyncURIMatcher::prefix(kBlobsPrefix), HTTP_OPTIONS,
              [store](AsyncWebServerRequest* request) { BlobController(request, Action::Options, store).dispatch(); });

    server.on(
        AsyncURIMatcher::prefix(kBlobsPrefix), HTTP_PUT, [](AsyncWebServerRequest* request) { (void)request; }, nullptr,
        [store](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            if (index == 0U) {
                auto* state = new BlobPutState();
                request->_tempObject = state;
                request->onDisconnect([request]() {
                    if (auto* leftover = static_cast<BlobPutState*>(request->_tempObject)) {
                        delete leftover;
                        request->_tempObject = nullptr;
                    }
                });

                std::string key;
                if (total > kBlobControllerMaxPutBytes) {
                    state->outcome = PutOutcome::TooLarge;
                } else if (!parseBlobKey(request->url().c_str(), key)) {
                    state->outcome = PutOutcome::BadKey;
                } else {
                    state->handle = store->beginPut(key);
                    if (!state->handle.valid()) {
                        state->outcome = PutOutcome::WriteFailed;
                    }
                }
            }

            auto* state = static_cast<BlobPutState*>(request->_tempObject);
            if (state == nullptr) {
                return;
            }

            if (state->outcome == PutOutcome::Ok && len > 0U) {
                if (!state->handle.write(data, len)) {
                    state->outcome = PutOutcome::WriteFailed;
                }
            }

            if (index + len >= total) {
                PutOutcome outcome = state->outcome;
                if (outcome == PutOutcome::Ok && !state->handle.commit()) {
                    outcome = PutOutcome::WriteFailed;
                }
                // Must delete + null out _tempObject BEFORE constructing the controller: BaseController's
                // cleanupRequestState() would otherwise misinterpret or double-free this `new`'d pointer.
                delete state;
                request->_tempObject = nullptr;
                BlobController(request, Action::Update, store, outcome).dispatch();
            }
        });

    server.on(
        AsyncURIMatcher::prefix(kBlobsPrefix), HTTP_POST, [](AsyncWebServerRequest* request) { (void)request; }, nullptr,
        [store](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            if (index == 0U) {
                auto* state = new BlobPostState();
                request->_tempObject = state;
                request->onDisconnect([request]() {
                    if (auto* leftover = static_cast<BlobPostState*>(request->_tempObject)) {
                        delete leftover;
                        request->_tempObject = nullptr;
                    }
                });

                std::string prefix;
                if (total > kBlobControllerMaxPutBytes) {
                    state->outcome = PutOutcome::TooLarge;
                } else if (!parseBlobKey(request->url().c_str(), prefix)) {
                    state->outcome = PutOutcome::BadKey;
                } else {
                    state->handle = store->beginPutGenerated(prefix, state->key);
                    if (!state->handle.valid()) {
                        state->outcome = state->key.empty() ? PutOutcome::KeyGenFailed : PutOutcome::WriteFailed;
                    }
                }
            }

            auto* state = static_cast<BlobPostState*>(request->_tempObject);
            if (state == nullptr) {
                return;
            }

            if (state->outcome == PutOutcome::Ok && len > 0U) {
                if (!state->handle.write(data, len)) {
                    state->outcome = PutOutcome::WriteFailed;
                }
            }

            if (index + len >= total) {
                PutOutcome outcome = state->outcome;
                if (outcome == PutOutcome::Ok && !state->handle.commit()) {
                    outcome = PutOutcome::WriteFailed;
                }
                std::string generatedKey = outcome == PutOutcome::Ok ? std::move(state->key) : std::string{};
                // Must delete + null out _tempObject BEFORE constructing the controller: BaseController's
                // cleanupRequestState() would otherwise misinterpret or double-free this `new`'d pointer.
                delete state;
                request->_tempObject = nullptr;
                BlobController(request, Action::Update, store, outcome, std::move(generatedKey)).dispatch();
            }
        });
}

#endif

} // namespace ewfm
