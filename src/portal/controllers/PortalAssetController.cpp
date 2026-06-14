#include "portal/controllers/PortalAssetController.h"

#if defined(ARDUINO) && !defined(UNIT_TEST)
#include "debug/Debug.h"

#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#endif

namespace ewfm {

#if defined(ARDUINO) && !defined(UNIT_TEST)
namespace {
constexpr const char* kIndexPath = "/index.html.gz";
constexpr const char* kIndexMime = "text/html";
constexpr const char* kNotFoundJson = "{\"success\":false,\"code\":\"NOT_FOUND\",\"error\":\"not found\"}";
} // namespace

void PortalAssetController::registerRoutes(AsyncWebServer& server) {
    server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) { handleIndex(request); });
    server.on("/index.html", HTTP_GET, [](AsyncWebServerRequest* request) { handleIndex(request); });
    server.on(AsyncURIMatcher::prefix("/assets/"), HTTP_GET, [](AsyncWebServerRequest* request) { handleAsset(request); });
    server.onNotFound([](AsyncWebServerRequest* request) { handleFallback(request); });
}

void PortalAssetController::handleIndex(AsyncWebServerRequest* request) {
    if (request == nullptr) {
        return;
    }
    if (!LittleFS.exists(kIndexPath)) {
        send404(request);
        return;
    }

    AsyncWebServerResponse* response = request->beginResponse(LittleFS, kIndexPath, kIndexMime);
    response->addHeader("Content-Encoding", "gzip");
    addNoCacheHeaders(response);
    request->send(response);
}

void PortalAssetController::handleAsset(AsyncWebServerRequest* request) {
    if (request == nullptr) {
        return;
    }

    const String path = request->url();
    const String normalizedPath = stripGzipSuffix(path);
    const String gzipPath = gzipAssetPath(normalizedPath);
    if (!LittleFS.exists(normalizedPath) && !LittleFS.exists(gzipPath)) {
        send404(request);
        return;
    }

    File asset = LittleFS.exists(gzipPath) ? LittleFS.open(gzipPath, "r") : LittleFS.open(normalizedPath, "r");
    if (!asset) {
        send404(request);
        return;
    }

    AsyncWebServerResponse* response = request->beginResponse(asset, normalizedPath, contentTypeForPath(normalizedPath));
    addImmutableHeaders(response);
    request->send(response);
}

void PortalAssetController::handleFallback(AsyncWebServerRequest* request) {
    if (request == nullptr) {
        return;
    }

    const String path = request->url();
    if (isApiPath(path) || isWebSocketPath(path)) {
        send404(request);
        return;
    }

    handleIndex(request);
}

bool PortalAssetController::isApiPath(const String& path) {
    return path.startsWith("/api/");
}

bool PortalAssetController::isWebSocketPath(const String& path) {
    return path == "/ws" || path.startsWith("/ws/");
}

String PortalAssetController::gzipAssetPath(const String& path) {
    return path + ".gz";
}

String PortalAssetController::stripGzipSuffix(const String& path) {
    if (!path.endsWith(".gz")) {
        return path;
    }

    return path.substring(0, path.length() - 3);
}

const char* PortalAssetController::contentTypeForPath(const String& path) {
    if (path.endsWith(".js")) {
        return "application/javascript";
    }
    if (path.endsWith(".css")) {
        return "text/css";
    }
    if (path.endsWith(".json")) {
        return "application/json";
    }
    if (path.endsWith(".svg")) {
        return "image/svg+xml";
    }
    if (path.endsWith(".png")) {
        return "image/png";
    }
    return "application/octet-stream";
}

void PortalAssetController::addNoCacheHeaders(AsyncWebServerResponse* response) {
    if (response == nullptr) {
        return;
    }
    response->addHeader("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
    response->addHeader("Pragma", "no-cache");
    response->addHeader("Expires", "0");
    response->addHeader("Accept-Ranges", "none");
}

void PortalAssetController::addImmutableHeaders(AsyncWebServerResponse* response) {
    if (response == nullptr) {
        return;
    }
    response->addHeader("Cache-Control", "public, max-age=31536000, immutable");
    response->addHeader("Accept-Ranges", "none");
}

void PortalAssetController::send404(AsyncWebServerRequest* request) {
    if (request == nullptr) {
        return;
    }
    AsyncWebServerResponse* response = request->beginResponse(404, "application/json", kNotFoundJson);
    response->addHeader("Accept-Ranges", "none");
    request->send(response);
}
#endif

} // namespace ewfm
