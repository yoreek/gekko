#include "portal/PortalServer.h"

#include "portal/routes/OtaPortalRoutes.h"
#include "portal/routes/PortalHomeRoutes.h"
#include "portal/routes/WifiPortalRoutes.h"

#if defined(ARDUINO) && !defined(UNIT_TEST)
#include "debug/Debug.h"

#include <DNSServer.h>
#include <ESPAsyncWebServer.h>
#include <WiFi.h>
#endif

namespace ewfm {

class PortalServer::Impl {
public:
    Impl(ProvisioningCoordinator& coordinator, IWifiDriver& wifiDriver)
        : coordinator_(coordinator), wifiDriver_(wifiDriver), homeRoutes_(new PortalHomeRoutes()),
          wifiRoutes_(new WifiPortalRoutes(coordinator_, wifiDriver_)), otaRoutes_(new OtaPortalRoutes()) {}

    bool begin() {
#if defined(ARDUINO) && !defined(UNIT_TEST)
        if (server_) {
            return true;
        }

        server_ = std::make_unique<AsyncWebServer>(80);
        dns_ = std::make_unique<DNSServer>();
        dns_->start(53, "*", WiFi.softAPIP());

        homeRoutes_->registerRoutes(*server_);
        wifiRoutes_->registerRoutes(*server_);
        otaRoutes_->registerRoutes(*server_);

        server_->begin();
        EWFM_PORTAL_LOG_INFO("portal started");
        return true;
#else
        return false;
#endif
    }

    void end() {
#if defined(ARDUINO) && !defined(UNIT_TEST)
        if (server_) {
            server_->end();
            server_.reset();
        }
        if (dns_) {
            dns_->stop();
            dns_.reset();
        }
        EWFM_PORTAL_LOG_INFO("portal stopped");
#endif
    }

    void tick() {
#if defined(ARDUINO) && !defined(UNIT_TEST)
        if (dns_) {
            dns_->processNextRequest();
        }
#endif
    }

private:
#if defined(ARDUINO) && !defined(UNIT_TEST)
    std::unique_ptr<AsyncWebServer> server_;
    std::unique_ptr<DNSServer> dns_;
#endif
    ProvisioningCoordinator& coordinator_;
    IWifiDriver& wifiDriver_;
    std::unique_ptr<PortalHomeRoutes> homeRoutes_;
    std::unique_ptr<WifiPortalRoutes> wifiRoutes_;
    std::unique_ptr<OtaPortalRoutes> otaRoutes_;
};

PortalServer::PortalServer(ProvisioningCoordinator& coordinator, IWifiDriver& wifiDriver) : impl_(new Impl(coordinator, wifiDriver)) {}

PortalServer::~PortalServer() = default;

bool PortalServer::begin() {
    return impl_->begin();
}

void PortalServer::end() {
    impl_->end();
}

void PortalServer::tick() {
    impl_->tick();
}

} // namespace ewfm
