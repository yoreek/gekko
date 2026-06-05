#pragma once

#if defined(ARDUINO) && !defined(UNIT_TEST)
class AsyncWebServer;
#endif

namespace ewfm {

class PortalHomeRoutes {
public:
#if defined(ARDUINO) && !defined(UNIT_TEST)
    void registerRoutes(AsyncWebServer& server);
#endif
};

} // namespace ewfm
