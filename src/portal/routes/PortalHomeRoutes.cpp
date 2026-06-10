#include "portal/routes/PortalHomeRoutes.h"

#if defined(ARDUINO) && !defined(UNIT_TEST)
#include "portal/controllers/PortalAssetController.h"

#include <ESPAsyncWebServer.h>
#endif

namespace ewfm {

#if defined(ARDUINO) && !defined(UNIT_TEST)
void PortalHomeRoutes::registerRoutes(AsyncWebServer& server) {
    PortalAssetController::registerRoutes(server);
}
#endif

} // namespace ewfm
