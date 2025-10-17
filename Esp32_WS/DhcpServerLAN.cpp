#include "DhcpServerLAN.h"

// BOOTP / DHCP Offsets & Magic Cookie
#define BOOTP_OP        0
#define BOOTP_HTYPE     1
#define BOOTP_HLEN      2
#define BOOTP_HOPS      3
#define BOOTP_XID       4
#define BOOTP_CIADDR    12
#define BOOTP_YIADDR    16
#define BOOTP_CHADDR    28
#define BOOTP_OPTIONS   236

static const uint8_t DHCP_MAGIC[4] = {99, 130, 83, 99};

void DhcpServerLAN::begin(const IPAddress &serverIp) {
  _serverIp = serverIp;
  _gateway = serverIp;
  _dns = serverIp;

  if (_poolEnd >= _poolStart) {
    _poolSize = _poolEnd - _poolStart + 1;
  } else {
    _poolSize = 0;
  }

  if (leases) {
    free(leases);
  }
  leases = (Lease*) malloc(sizeof(Lease) * _poolSize);
  for (int i = 0; i < _poolSize; i++) {
    leases[i].used = false;
  }

  // Beispiel MAC-Adresse (einmalig und eindeutig wählen!)
  static byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

  Ethernet.init(W5500_SS);
  Ethernet.begin(mac, _serverIp, _gateway, _dns, _netmask);

  if (_udp.begin(LAN_DHCP_SERVER_PORT) != 1) {
    Serial.println("DHCP UDP Start fehlgeschlagen");
  // } else {
  //   Serial.print("LAN gestartet. IP-Adresse: ");
  //   Serial.println(_serverIp);
  }
}

void DhcpServerLAN::loop() {
  cleanupLeases();
  int packetSize = _udp.parsePacket();
  if (packetSize > 0 && packetSize <= LAN_DHCP_MAX_LEN) {
    _udp.read(dhcpBuf, packetSize);
    handlePacket();
  }
}

void DhcpServerLAN::cleanupLeases() {
  uint32_t now = millis() / 1000;
  for (int i = 0; i < _poolSize; i++) {
    if (leases[i].used) {
      if ((now - leases[i].leaseStart) >= leases[i].leaseSeconds) {
        leases[i].used = false;
      }
    }
  }
}

bool DhcpServerLAN::macEqual(const uint8_t *a, const uint8_t *b) {
  for (int i = 0; i < 6; i++) {
    if (a[i] != b[i]) return false;
  }
  return true;
}

int DhcpServerLAN::findLeaseByMac(const uint8_t *mac) {
  for (int i = 0; i < _poolSize; i++) {
    if (leases[i].used && macEqual(leases[i].mac, mac)) {
      return i;
    }
  }
  return -1;
}

int DhcpServerLAN::allocateLease(const uint8_t *mac) {
  int idx = findLeaseByMac(mac);
  if (idx >= 0) {
    leases[idx].leaseStart = millis() / 1000;
    leases[idx].leaseSeconds = _leaseTime;
    return idx;
  }
  for (int i = 0; i < _poolSize; i++) {
    if (!leases[i].used) {
      leases[i].used = true;
      memcpy(leases[i].mac, mac, 6);
      leases[i].ip = poolIndexToIP(i);
      leases[i].leaseStart = millis() / 1000;
      leases[i].leaseSeconds = _leaseTime;
      return i;
    }
  }
  return -1;
}

IPAddress DhcpServerLAN::poolIndexToIP(int idx) {
  return IPAddress(_serverIp[0], _serverIp[1], _serverIp[2], _poolStart + idx);
}

void DhcpServerLAN::handlePacket() {
  uint8_t op = dhcpBuf[BOOTP_OP];
  if (op != 1) return; // nur BOOTREQUEST

  // Prüfe Magic Cookie
  if (!(dhcpBuf[BOOTP_OPTIONS] == DHCP_MAGIC[0] &&
        dhcpBuf[BOOTP_OPTIONS+1] == DHCP_MAGIC[1] &&
        dhcpBuf[BOOTP_OPTIONS+2] == DHCP_MAGIC[2] &&
        dhcpBuf[BOOTP_OPTIONS+3] == DHCP_MAGIC[3])) {
    // Ignorieren oder evtl. trotzdem verarbeiten
  }

  uint8_t clientMac[6];
  memcpy(clientMac, &dhcpBuf[BOOTP_CHADDR], 6);

  // DHCP Message Type (Option 53)
  int idx = BOOTP_OPTIONS + 4;
  uint8_t msgType = 0;
  while (idx < LAN_DHCP_MAX_LEN) {
    uint8_t opt = dhcpBuf[idx++];
    if (opt == 0) continue;
    if (opt == 255) break;
    if (idx >= LAN_DHCP_MAX_LEN) break;
    uint8_t len = dhcpBuf[idx++];
    if (opt == 53 && len >= 1) {
      msgType = dhcpBuf[idx];
      break;
    }
    idx += len;
  }

  if (msgType == 1) {  // DHCPDISCOVER
    int li = allocateLease(clientMac);
    if (li >= 0) {
      sendDhcpReply(dhcpBuf, 2, leases[li].ip);  // DHCPOFFER
    }
  } else if (msgType == 3) {  // DHCPREQUEST
    int li = allocateLease(clientMac);
    if (li >= 0) {
      sendDhcpReply(dhcpBuf, 5, leases[li].ip);  // DHCPACK
    }
  }
  // Andere Typen ignorieren
}

void DhcpServerLAN::sendDhcpReply(const uint8_t *request, uint8_t replyType, const IPAddress &yiaddr) {
  uint8_t resp[LAN_DHCP_MAX_LEN];
  memset(resp, 0, LAN_DHCP_MAX_LEN);

  resp[BOOTP_OP] = 2;  // BOOTREPLY
  resp[BOOTP_HTYPE] = request[BOOTP_HTYPE];
  resp[BOOTP_HLEN] = request[BOOTP_HLEN];
  memcpy(&resp[BOOTP_XID], &request[BOOTP_XID], 4);

  // yiaddr (Client IP)
  uint32_t ip = (yiaddr[0] << 24) | (yiaddr[1] << 16) | (yiaddr[2] << 8) | yiaddr[3];
  resp[16] = (ip >> 24) & 0xFF;
  resp[17] = (ip >> 16) & 0xFF;
  resp[18] = (ip >> 8) & 0xFF;
  resp[19] = ip & 0xFF;

  // chaddr (Client MAC)
  memcpy(&resp[BOOTP_CHADDR], &request[BOOTP_CHADDR], 16);

  // Magic Cookie
  memcpy(&resp[BOOTP_OPTIONS], DHCP_MAGIC, 4);
  int opt = BOOTP_OPTIONS + 4;

  // Option 53: Message Type
  resp[opt++] = 53; resp[opt++] = 1; resp[opt++] = replyType;

  // Option 54: Server Identifier
  resp[opt++] = 54; resp[opt++] = 4;
  resp[opt++] = _serverIp[0]; resp[opt++] = _serverIp[1];
  resp[opt++] = _serverIp[2]; resp[opt++] = _serverIp[3];

  // Option 1: Subnet Mask
  resp[opt++] = 1; resp[opt++] = 4;
  resp[opt++] = _netmask[0]; resp[opt++] = _netmask[1];
  resp[opt++] = _netmask[2]; resp[opt++] = _netmask[3];

  // Option 3: Router / Gateway
  resp[opt++] = 3; resp[opt++] = 4;
  resp[opt++] = _gateway[0]; resp[opt++] = _gateway[1];
  resp[opt++] = _gateway[2]; resp[opt++] = _gateway[3];

  // Option 6: DNS
  resp[opt++] = 6; resp[opt++] = 4;
  resp[opt++] = _dns[0]; resp[opt++] = _dns[1];
  resp[opt++] = _dns[2]; resp[opt++] = _dns[3];

  // Option 51: Lease Time
  resp[opt++] = 51; resp[opt++] = 4;
  uint32_t lt = _leaseTime;
  resp[opt++] = (lt >> 24) & 0xFF;
  resp[opt++] = (lt >> 16) & 0xFF;
  resp[opt++] = (lt >> 8) & 0xFF;
  resp[opt++] = lt & 0xFF;

  // End Option (255)
  resp[opt++] = 255;

  // Antwort per Broadcast an Port 68
  _udp.beginPacket(IPAddress(255,255,255,255), LAN_DHCP_CLIENT_PORT);
  _udp.write(resp, opt);
  _udp.endPacket();
}

void DhcpServerLAN::setPoolRange(uint8_t start, uint8_t end) {
  _poolStart = start;
  _poolEnd = end;
  if (_poolEnd >= _poolStart) {
    _poolSize = _poolEnd - _poolStart + 1;
  } else {
    _poolSize = 0;
  }

  if (leases) {
    free(leases);
  }
  leases = (Lease*) malloc(sizeof(Lease) * _poolSize);
  for (int i = 0; i < _poolSize; i++) {
    leases[i].used = false;
  }
}

void DhcpServerLAN::setLeaseTime(uint32_t seconds) {
  _leaseTime = seconds;
}
