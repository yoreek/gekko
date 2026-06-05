#include "portal/routes/PortalHomeRoutes.h"

#if defined(ARDUINO) && !defined(UNIT_TEST)
#include "portal/PortalAssets.h"

#include <ESPAsyncWebServer.h>
#endif

namespace ewfm {

#if defined(ARDUINO) && !defined(UNIT_TEST)
void PortalHomeRoutes::registerRoutes(AsyncWebServer& server) {
    server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) { request->send(200, "text/html", portalHtml()); });

    server.onNotFound([](AsyncWebServerRequest* request) { request->redirect("/"); });
}
#endif

} // namespace ewfm
