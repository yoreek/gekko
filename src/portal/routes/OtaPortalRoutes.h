#pragma once

#if defined(ARDUINO) && !defined(UNIT_TEST)
#include <WString.h>
#include <cstddef>
#include <cstdint>

class AsyncWebServer;
class AsyncWebServerRequest;
#endif

namespace ewfm {

class OtaPortalRoutes {
public:
#if defined(ARDUINO) && !defined(UNIT_TEST)
    void registerRoutes(AsyncWebServer& server);
#endif

private:
#if defined(ARDUINO) && !defined(UNIT_TEST)
    void handleStatus(AsyncWebServerRequest* request);
    void handleFinished(AsyncWebServerRequest* request);
    void handleUpload(AsyncWebServerRequest* request, const String& filename, size_t index, uint8_t* data, size_t len, bool final);
    bool beginUpload(size_t totalBytes);
#endif
};

} // namespace ewfm
