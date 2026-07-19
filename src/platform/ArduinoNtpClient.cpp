#include "platform/ArduinoNtpClient.h"

#if defined(ARDUINO) && !defined(UNIT_TEST)
#include <lwip/def.h>
#endif

namespace ewfm {

namespace {
constexpr uint16_t kLocalPort = 1337;
constexpr uint16_t kRemotePort = 123;
constexpr uint64_t kNtpToUnixEpochDeltaSeconds = 2208988800ULL;
} // namespace

void ArduinoNtpClient::beginResolve(const std::string& hostname) {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    resolveStatus_ = NtpResolveStatus::Pending;
    ip_addr_t addr;
    const err_t err = dns_gethostbyname(hostname.c_str(), &addr, &ArduinoNtpClient::dnsCallback, this);
    if (err == ERR_OK) {
        serverAddress_ = IPAddress(addr.u_addr.ip4.addr);
        resolveStatus_ = NtpResolveStatus::Resolved;
    } else if (err != ERR_INPROGRESS) {
        resolveStatus_ = NtpResolveStatus::Failed;
    }
#else
    (void)hostname;
    resolveStatus_ = NtpResolveStatus::Failed;
#endif
}

NtpResolveStatus ArduinoNtpClient::resolveStatus() const {
    return resolveStatus_;
}

bool ArduinoNtpClient::openSocket() {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    return udp_.begin(kLocalPort) != 0;
#else
    return false;
#endif
}

void ArduinoNtpClient::closeSocket() {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    udp_.stop();
#endif
}

bool ArduinoNtpClient::sendRequest() {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    flushPackets();
    sendNtpPacket();
    return true;
#else
    return false;
#endif
}

bool ArduinoNtpClient::pollResponse(uint32_t& outEpochSeconds) {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    const int size = udp_.parsePacket();
    if (size == 0) {
        return false;
    }
    if (size != static_cast<int>(sizeof(NtpPacket))) {
        return false;
    }

    NtpPacket packet{};
    udp_.read(reinterpret_cast<uint8_t*>(&packet), sizeof(packet));
    const uint32_t txSeconds = ntohl(packet.txTmS);
    outEpochSeconds = static_cast<uint32_t>(static_cast<uint64_t>(txSeconds) - kNtpToUnixEpochDeltaSeconds);
    return true;
#else
    (void)outEpochSeconds;
    return false;
#endif
}

#if defined(ARDUINO) && !defined(UNIT_TEST)
void ArduinoNtpClient::flushPackets() {
    while (udp_.parsePacket() != 0) {
        udp_.flush();
    }
}

void ArduinoNtpClient::sendNtpPacket() {
    NtpPacket packet{};
    packet.liVnMode = 0b11100011;
    packet.poll = 6;
    packet.precision = 0xEC;
    packet.refId = 0x34314E31; // "41N1"

    udp_.beginPacket(serverAddress_, kRemotePort);
    udp_.write(reinterpret_cast<const uint8_t*>(&packet), sizeof(packet));
    udp_.endPacket();
}

void ArduinoNtpClient::handleDnsResult(const ip_addr_t* ipaddr) {
    if (ipaddr == nullptr) {
        resolveStatus_ = NtpResolveStatus::Failed;
        return;
    }
    serverAddress_ = IPAddress(ipaddr->u_addr.ip4.addr);
    resolveStatus_ = NtpResolveStatus::Resolved;
}

void ArduinoNtpClient::dnsCallback(const char* name, const ip_addr_t* ipaddr, void* arg) {
    (void)name;
    if (arg == nullptr) {
        return;
    }
    static_cast<ArduinoNtpClient*>(arg)->handleDnsResult(ipaddr);
}
#endif

} // namespace ewfm
