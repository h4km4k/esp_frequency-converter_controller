#ifndef DHCP_SERVER_LAN_H
#define DHCP_SERVER_LAN_H

#include <Arduino.h>
#include <ETH.h>
#include <AsyncUDP.h>

class DhcpServerLAN {
public:
    void begin(const IPAddress &serverIp);
    void loop();  // Optional, AsyncUDP benutzt Callback

    // Optional: Pool-Range (Standard 10-50)
    void setPoolRange(uint8_t start, uint8_t end);

    // Optional: Lease-Time in Sekunden (Standard 3600)
    void setLeaseTime(uint32_t seconds);

private:
    void cleanupLeases();
    void handlePacket();
    void sendDhcpReply(const uint8_t *request, uint8_t replyType, const IPAddress &yiaddr);

    static constexpr int LAN_DHCP_SERVER_PORT = 67;
    static constexpr int LAN_DHCP_CLIENT_PORT = 68;
    static constexpr int LAN_DHCP_MAX_LEN = 548;

    uint8_t dhcpBuf[LAN_DHCP_MAX_LEN];
    AsyncUDP _udp;

    IPAddress _serverIp;
    IPAddress _netmask = IPAddress(255, 255, 255, 0);
    IPAddress _gateway;
    IPAddress _dns;

    uint8_t _poolStart = 10;
    uint8_t _poolEnd = 50;
    uint32_t _leaseTime = 3600; // Sekunden

    struct Lease {
        bool used;
        uint8_t mac[6];
        IPAddress ip;
        uint32_t leaseStart;    // in Sekunden (Millis()/1000)
        uint32_t leaseSeconds;  // Lease-Dauer
    };

    Lease *leases = nullptr;
    uint8_t _poolSize = 0;

    int findLeaseByMac(const uint8_t *mac);
    int allocateLease(const uint8_t *mac);
    IPAddress poolIndexToIP(int idx);
    bool macEqual(const uint8_t *a, const uint8_t *b);
};

#endif  // DHCP_SERVER_LAN_H
