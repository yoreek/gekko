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
        homeRoutes_->registerRoutes(*server_);
        wifiRoutes_->registerRoutes(*server_);
        otaRoutes_->registerRoutes(*server_);

        server_->begin();
        EWFM_PORTAL_LOG_INFO("portal started");
        startDnsIfNeeded();
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

    void tick(uint32_t now) {
        (void)now;
#if defined(ARDUINO) && !defined(UNIT_TEST)
        if (wifiDriver_.setupApActive()) {
            startDnsIfNeeded();
        } else if (dns_) {
            dns_->stop();
            dns_.reset();
            EWFM_PORTAL_LOG_INFO("portal dns stopped");
        }
        if (dns_) {
            dns_->processNextRequest();
        }
#endif
    }

private:
#if defined(ARDUINO) && !defined(UNIT_TEST)
    void startDnsIfNeeded() {
        if (!server_) {
            return;
        }

        if (!wifiDriver_.setupApActive()) {
            if (dns_) {
                dns_->stop();
                dns_.reset();
                EWFM_PORTAL_LOG_INFO("portal dns stopped");
            }
            return;
        }

        const IPAddress apIp = parseIp(wifiDriver_.setupApIp());
        if (apIp != IPAddress(0, 0, 0, 0) && !dns_) {
            dns_ = std::make_unique<DNSServer>();
            dns_->start(53, "*", apIp);
            EWFM_PORTAL_LOG_INFO("portal dns started");
            return;
        }
        if (apIp == IPAddress(0, 0, 0, 0) && dns_) {
            dns_->stop();
            dns_.reset();
            EWFM_PORTAL_LOG_INFO("portal dns stopped");
        }
    }

    static IPAddress parseIp(const std::string& value) {
        IPAddress ip;
        if (ip.fromString(value.c_str())) {
            return ip;
        }
        return IPAddress(0, 0, 0, 0);
    }

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

void PortalServer::tick(uint32_t now) {
    impl_->tick(now);
}

} // namespace ewfm
