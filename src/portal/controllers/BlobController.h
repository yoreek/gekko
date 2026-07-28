#pragma once

#include "platform/LittleFsBlobStore.h"
#include "portal/controllers/BaseController.h"

#include <string>

#if defined(ARDUINO) && !defined(UNIT_TEST)
class AsyncWebServer;
class AsyncWebServerRequest;
#endif

namespace ewfm {

// Generic REST surface over LittleFsBlobStore - one controller for every feature that needs
// key/blob storage, not one controller per file type:
//   GET    /api/blobs/<key>    - fetch the stored blob's raw bytes
//   PUT    /api/blobs/<key>    - upload/replace a blob at a caller-chosen key (raw binary body)
//   POST   /api/blobs/<prefix> - upload a blob under a caller-chosen prefix, with the unique part
//                                of the key generated server-side; responds with the full
//                                generated key. For untrusted callers (e.g. the SPA) that must
//                                never be able to choose/collide the unique part of a key.
//   DELETE /api/blobs/<key>    - remove the blob at <key>, or everything under it if <key> names
//                                a directory-shaped prefix (removeByPrefix handles both)
// <key>/<prefix> is everything after "/api/blobs/", including embedded '/' - validated via
// isValidBlobKey before ever touching the filesystem.
class BlobController : public BaseController {
public:
    // Outcome of a PUT/POST's chunked write, computed by the body-handler callback across chunks
    // and handed to the controller once the stream completes (dispatch happens on the final chunk).
    enum class PutOutcome : uint8_t {
        Ok,
        BadKey,
        TooLarge,
        WriteFailed,
        KeyGenFailed, // POST only: could not find a free generated key within the attempt budget
    };

    BlobController(AsyncWebServerRequest* request, Action action, LittleFsBlobStore* store, PutOutcome putOutcome = PutOutcome::Ok,
                   std::string generatedKey = {});

#if defined(ARDUINO) && !defined(UNIT_TEST)
    static void registerRoutes(AsyncWebServer& server, LittleFsBlobStore* store);
#endif

#if defined(UNIT_TEST)
    static bool parseBlobKeyForTest(const char* url, std::string& key) {
        return parseBlobKey(url, key);
    }
#endif

protected:
    const RulesChain* beforeChain() override;
    void show() override;
    void update() override;
    void destroy() override;
    void options() override;

private:
    static bool parseBlobKey(const char* url, std::string& key);
    static bool requireBlobKey(BaseController& self);

    LittleFsBlobStore* store_;
    PutOutcome putOutcome_;
    std::string key_{};
    std::string generatedKey_{}; // POST only: the server-generated key to echo back in the response
};

} // namespace ewfm
