#include "portal/PortalServer.h"

#include "core/StateMachine.h"
#include "debug/Debug.h"
#include "portal/routes/OtaPortalRoutes.h"
#include "portal/routes/PortalHomeRoutes.h"
#include "portal/routes/WifiPortalRoutes.h"

#if defined(ARDUINO) && !defined(UNIT_TEST)
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>
#include <WiFi.h>
#endif

namespace ewfm {

#undef SM_CLASS
#define SM_CLASS PortalServer::Impl

class PortalServer::Impl : public StateMachine {
public:
    Impl(WifiManager& wifiManager, IWifiDriver& wifiDriver)
        : StateMachine((PState)&PortalServer::Impl::Idle), wifiManager_(wifiManager), wifiDriver_(wifiDriver) {}

    bool begin() {
        configured_ = true;
        return true;
    }

    void end() {
        stopDns();
        stopHttp();
        configured_ = false;
        setState((PState)&PortalServer::Impl::Idle, uptime());
    }

    PortalRuntimeState state() const {
        if (is((PState)&PortalServer::Impl::Idle)) {
            return PortalRuntimeState::Idle;
        }
        if (is((PState)&PortalServer::Impl::WaitingForNetwork)) {
            return PortalRuntimeState::WaitingForNetwork;
        }
        if (is((PState)&PortalServer::Impl::Starting)) {
            return PortalRuntimeState::Starting;
        }
        if (is((PState)&PortalServer::Impl::Running)) {
            return PortalRuntimeState::Running;
        }
        return PortalRuntimeState::Faulted;
    }

    bool httpRunning() const {
        return httpRunning_;
    }

    bool dnsRunning() const {
        return dnsRunning_;
    }

#if defined(UNIT_TEST)
    uint16_t httpStartCount() const {
        return httpStartCount_;
    }
    uint16_t httpStopCount() const {
        return httpStopCount_;
    }
    uint16_t dnsStartCount() const {
        return dnsStartCount_;
    }
    uint16_t dnsStopCount() const {
        return dnsStopCount_;
    }
#endif

    void Idle();
    void WaitingForNetwork();
    void Starting();
    void Running();
    void Faulted();

private:
    bool startHttp() {
        if (httpRunning_) {
            return true;
        }

#if defined(ARDUINO) && !defined(UNIT_TEST)
        server_ = std::make_unique<AsyncWebServer>(80);
        if (!server_) {
            EWFM_PORTAL_LOG_WARN("portal http allocation failed");
            return false;
        }
        if (!homeRoutes_) {
            homeRoutes_ = std::make_unique<PortalHomeRoutes>();
        }
        if (!wifiRoutes_) {
            wifiRoutes_ = std::make_unique<WifiPortalRoutes>(wifiManager_, wifiDriver_);
        }
        if (!otaRoutes_) {
            otaRoutes_ = std::make_unique<OtaPortalRoutes>();
        }
        homeRoutes_->registerRoutes(*server_);
        wifiRoutes_->registerRoutes(*server_);
        otaRoutes_->registerRoutes(*server_);

        server_->begin();
#endif
        httpRunning_ = true;
        ++httpStartCount_;
        EWFM_PORTAL_LOG_INFO("portal started");
        return true;
    }

    void stopHttp() {
        stopDns();
        if (!httpRunning_) {
            return;
        }

#if defined(ARDUINO) && !defined(UNIT_TEST)
        if (server_) {
            server_->end();
            server_.reset();
        }
#endif
        httpRunning_ = false;
        ++httpStopCount_;
        EWFM_PORTAL_LOG_INFO("portal stopped");
    }

    void updateDns() {
        if (!httpRunning_ || !wifiDriver_.setupApReady()) {
            stopDns();
            return;
        }

#if defined(ARDUINO) && !defined(UNIT_TEST)
        const IPAddress apIp = parseIp(wifiDriver_.setupApIp());
        if (apIp == IPAddress(0, 0, 0, 0)) {
            stopDns();
            return;
        }

        if (dns_) {
            dns_->processNextRequest();
            return;
        }

        dns_ = std::make_unique<DNSServer>();
        dns_->start(53, "*", apIp);
#endif
        if (!dnsRunning_) {
            dnsRunning_ = true;
            ++dnsStartCount_;
            EWFM_PORTAL_LOG_INFO("portal dns started");
        }
    }

    void stopDns() {
        if (!dnsRunning_) {
            return;
        }

#if defined(ARDUINO) && !defined(UNIT_TEST)
        if (dns_) {
            dns_->stop();
            dns_.reset();
        }
#endif
        dnsRunning_ = false;
        ++dnsStopCount_;
        EWFM_PORTAL_LOG_INFO("portal dns stopped");
    }

#if defined(ARDUINO) && !defined(UNIT_TEST)
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
    WifiManager& wifiManager_;
    IWifiDriver& wifiDriver_;
    std::unique_ptr<PortalHomeRoutes> homeRoutes_;
    std::unique_ptr<WifiPortalRoutes> wifiRoutes_;
    std::unique_ptr<OtaPortalRoutes> otaRoutes_;
    bool configured_{false};
    bool dependencyWaitLogged_{false};
    bool httpRunning_{false};
    bool dnsRunning_{false};
    uint16_t httpStartCount_{0};
    uint16_t httpStopCount_{0};
    uint16_t dnsStartCount_{0};
    uint16_t dnsStopCount_{0};
};

SM_STATE(Idle) {
    if (!configured_) {
        return;
    }
    SM_GOTO(WaitingForNetwork);
}

SM_STATE(WaitingForNetwork) {
    if (!configured_) {
        SM_GOTO(Idle);
    }
    if (!wifiDriver_.networkStackReady()) {
        if (!dependencyWaitLogged_) {
            EWFM_PORTAL_LOG_INFO("portal waiting for network stack");
            dependencyWaitLogged_ = true;
        }
        return;
    }

    dependencyWaitLogged_ = false;
    SM_GOTO(Starting);
}

SM_STATE(Starting) {
    if (!wifiDriver_.networkStackReady()) {
        SM_GOTO(WaitingForNetwork);
    }

    if (!startHttp()) {
        SM_GOTO(Faulted);
    }

    SM_GOTO(Running);
}

SM_STATE(Running) {
    if (!configured_) {
        stopHttp();
        SM_GOTO(Idle);
    }

    if (!wifiDriver_.networkStackReady()) {
        stopHttp();
        SM_GOTO(WaitingForNetwork);
    }

    updateDns();
}

SM_STATE(Faulted) {
    if (!configured_) {
        stopHttp();
        SM_GOTO(Idle);
    }

    if (!wifiDriver_.networkStackReady()) {
        stopHttp();
        SM_GOTO(WaitingForNetwork);
    }

    EWFM_PORTAL_LOG_WARN("portal faulted, retrying start");
    SM_GOTO(Starting);
}

PortalServer::PortalServer(WifiManager& wifiManager, IWifiDriver& wifiDriver) : wifiManager_(wifiManager), wifiDriver_(wifiDriver) {}

PortalServer::~PortalServer() = default;

bool PortalServer::begin() {
    if (!impl_) {
        impl_ = std::make_unique<Impl>(wifiManager_, wifiDriver_);
    }
    return impl_ != nullptr && impl_->begin();
}

void PortalServer::end() {
    impl_->end();
}

void PortalServer::tick(uint32_t now) {
    impl_->tick(now);
}

PortalRuntimeState PortalServer::state() const {
    return impl_->state();
}

bool PortalServer::httpRunning() const {
    return impl_->httpRunning();
}

bool PortalServer::dnsRunning() const {
    return impl_->dnsRunning();
}

#if defined(UNIT_TEST)
uint16_t PortalServer::httpStartCount() const {
    return impl_->httpStartCount();
}

uint16_t PortalServer::httpStopCount() const {
    return impl_->httpStopCount();
}

uint16_t PortalServer::dnsStartCount() const {
    return impl_->dnsStartCount();
}

uint16_t PortalServer::dnsStopCount() const {
    return impl_->dnsStopCount();
}
#endif

} // namespace ewfm
