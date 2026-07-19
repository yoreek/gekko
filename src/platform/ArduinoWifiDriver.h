#pragma once

#include "wifi/WifiDriver.h"

namespace ewfm {

class ArduinoWifiDriver final : public IWifiDriver {
public:
    bool begin() override;
    bool beginStation(const WiFiCredentials& credentials) override;
    void disconnect() override;
    void clearStationCredentials() override;
    bool startSetupAp(const std::string& ssid, const std::string& password) override;
    void stopSetupAp() override;
    WifiDriverStatus status() const override;
    bool networkStackReady() const override;
    bool stationReady() const override;
    bool setupApReady() const override;
    std::string stationIp() const override;
    std::string setupApIp() const override;
    bool startScan() override;
    bool scanComplete(std::vector<WifiNetwork>& networks, size_t maxResults) override;
    std::string macSuffix() const override;
    int32_t rssi() const override;
    std::string ssid() const override;

private:
    static bool ipValid(const std::string& ip);

    bool networkStackReady_{false};
};

} // namespace ewfm
