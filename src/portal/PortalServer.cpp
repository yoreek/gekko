#include "portal/PortalServer.h"

#include "core/StateMachine.h"
#include "debug/Debug.h"
#include "portal/controllers/DashboardLayoutController.h"
#include "portal/controllers/DeviceRegistryController.h"
#include "portal/controllers/OtaController.h"
#include "portal/controllers/PortalAssetController.h"
#include "portal/controllers/SystemController.h"
#include "portal/controllers/WifiController.h"
#include "portal/ws/PortalWebSocketManager.h"

#if defined(ARDUINO) && !defined(UNIT_TEST)
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>
#include <Update.h>
#include <WiFi.h>
#endif

namespace ewfm {

#undef SM_CLASS
#define SM_CLASS PortalServer::Impl

class PortalServer::Impl : public StateMachine {
public:
    Impl(WifiManager& wifiManager, IWifiDriver& wifiDriver, DeviceRegistry* deviceRegistry, DeviceEventDispatcher* deviceEventDispatcher,
         DashboardLayoutStore* dashboardLayoutStore)
        : StateMachine((PState)&PortalServer::Impl::Idle), wifiManager_(wifiManager), wifiDriver_(wifiDriver),
          deviceRegistry_(deviceRegistry), deviceEventDispatcher_(deviceEventDispatcher), dashboardLayoutStore_(dashboardLayoutStore) {}

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
        PortalAssetController::registerRoutes(*server_);
        WifiController::registerRoutes(*server_, wifiManager_, wifiDriver_);
        if (deviceRegistry_ != nullptr) {
            DeviceRegistryController::registerRoutes(*server_, *deviceRegistry_);
        }
        if (dashboardLayoutStore_ != nullptr) {
            DashboardLayoutController::registerRoutes(*server_, *dashboardLayoutStore_);
        }
        OtaController::registerRoutes(*server_, deviceRegistry_);
        SystemController::registerRoutes(*server_, deviceRegistry_);
        if (!webSocketManager_) {
            webSocketManager_ = std::make_unique<PortalWebSocketManager>(deviceEventDispatcher_);
        }
        if (webSocketManager_ != nullptr) {
            if (!webSocketManager_->begin(*server_)) {
                EWFM_PORTAL_LOG_WARN("portal websocket allocation failed");
                return false;
            }
        }

        server_->begin();
#endif
#if !defined(ARDUINO) || defined(UNIT_TEST)
        if (!webSocketManager_) {
            webSocketManager_ = std::make_unique<PortalWebSocketManager>(deviceEventDispatcher_);
        }
        if (webSocketManager_ != nullptr) {
            webSocketManager_->attachDispatcher();
        }
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
            if (webSocketManager_) {
                webSocketManager_->end(*server_);
            }
            server_->end();
            server_.reset();
        }
#else
        if (webSocketManager_) {
            webSocketManager_->detachDispatcher();
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
    DeviceRegistry* deviceRegistry_{nullptr};
    DeviceEventDispatcher* deviceEventDispatcher_{nullptr};
    DashboardLayoutStore* dashboardLayoutStore_{nullptr};
    std::unique_ptr<PortalWebSocketManager> webSocketManager_;
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
    if (webSocketManager_ != nullptr) {
#if defined(WITH_WEB_OTA)
        webSocketManager_->tick(uptime(), wifiManager_, wifiDriver_, true, Update.hasError(), ESP.getFreeSketchSpace());
#else
        webSocketManager_->tick(uptime(), wifiManager_, wifiDriver_, false, false, 0U);
#endif
    }
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

PortalServer::PortalServer(WifiManager& wifiManager, IWifiDriver& wifiDriver, DeviceRegistry* deviceRegistry,
                           DeviceEventDispatcher* deviceEventDispatcher, DashboardLayoutStore* dashboardLayoutStore)
    : wifiManager_(wifiManager), wifiDriver_(wifiDriver), deviceRegistry_(deviceRegistry), deviceEventDispatcher_(deviceEventDispatcher),
      dashboardLayoutStore_(dashboardLayoutStore) {}

PortalServer::~PortalServer() = default;

bool PortalServer::begin() {
    if (!impl_) {
        impl_ = std::make_unique<Impl>(wifiManager_, wifiDriver_, deviceRegistry_, deviceEventDispatcher_, dashboardLayoutStore_);
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
