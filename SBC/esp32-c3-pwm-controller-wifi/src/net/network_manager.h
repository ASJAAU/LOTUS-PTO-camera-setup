#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <Arduino.h>
#include "net/network_transport.h"
#include "net/network_server.h"
#include <memory>


class NetworkManager {
public:
  NetworkManager();
  void begin();
  void loop();
  bool   connected()   const;
  String localIP()     const;
  String getSSID()     const;   // SSID for WiFi; "Ethernet" for wired.
  int    rssi()        const;
  std::unique_ptr<NetServer> createServer(uint16_t port);

private:
  std::unique_ptr<INetworkTransport> _transport;
};

// Global instance — mirrors the pattern used by the old WiFiManager.
extern NetworkManager networkManager;

#endif // NETWORK_MANAGER_H
