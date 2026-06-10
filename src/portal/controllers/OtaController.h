#pragma once

#include "devices/registry/DeviceRegistry.h"
#include "portal/controllers/BaseController.h"

#if defined(ARDUINO) && !defined(UNIT_TEST)
class AsyncWebServer;
class AsyncWebServerRequest;
class String;
#endif

namespace ewfm {

class OtaController : public BaseController {
public:
    explicit OtaController(AsyncWebServerRequest* request, Action action, DeviceRegistry* deviceRegistry = nullptr);

#if defined(ARDUINO) && !defined(UNIT_TEST)
    static void registerRoutes(AsyncWebServer& server, DeviceRegistry* deviceRegistry = nullptr);
#endif

protected:
    void show() override;
    void create() override;

private:
    DeviceRegistry* deviceRegistry_{nullptr};

#if defined(ARDUINO) && !defined(UNIT_TEST)
    void handleUpload(const String& filename, size_t index, uint8_t* data, size_t len, bool final);
    bool beginUpload(size_t totalBytes);
#endif
};

} // namespace ewfm
