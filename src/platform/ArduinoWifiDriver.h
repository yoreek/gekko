#pragma once

#include "wifi/WifiDriver.h"

namespace ewfm {

class ArduinoWifiDriver final : public IWifiDriver {
public:
    bool beginStation(const WiFiCredentials& credentials) override;
    void disconnect() override;
    bool startSetupAp(const std::string& ssid, const std::string& password) override;
    void stopSetupAp() override;
    WifiDriverStatus status() const override;
    bool startScan() override;
    bool scanComplete(std::vector<WifiNetwork>& networks, size_t maxResults) override;
    std::string macSuffix() const override;
};

} // namespace ewfm
