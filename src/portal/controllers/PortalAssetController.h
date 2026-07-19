#pragma once

#if defined(ARDUINO) && !defined(UNIT_TEST)
class AsyncWebServer;
class AsyncWebServerRequest;
class AsyncWebServerResponse;
class String;
#endif

namespace ewfm {

class PortalAssetController {
public:
#if defined(ARDUINO) && !defined(UNIT_TEST)
    static void registerRoutes(AsyncWebServer& server);
#endif

private:
#if defined(ARDUINO) && !defined(UNIT_TEST)
    static void handleIndex(AsyncWebServerRequest* request);
    static void handleAsset(AsyncWebServerRequest* request);
    static void handleFallback(AsyncWebServerRequest* request);
    static bool isApiPath(const String& path);
    static bool isWebSocketPath(const String& path);
    static String gzipAssetPath(const String& path);
    static String stripGzipSuffix(const String& path);
    static const char* contentTypeForPath(const String& path);
    static void addNoCacheHeaders(AsyncWebServerResponse* response);
    static void addImmutableHeaders(AsyncWebServerResponse* response);
    static void send404(AsyncWebServerRequest* request);
#endif
};

} // namespace ewfm
