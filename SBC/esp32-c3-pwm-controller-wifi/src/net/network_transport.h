#ifndef NETWORK_TRANSPORT_H
#define NETWORK_TRANSPORT_H

#include <Arduino.h>
#include "net/network_server.h"
#include <memory>

// ---------------------------------------------------------------------------
// INetworkTransport — owns the link-layer connection (WiFi association or
// Ethernet cable) and can vend an NetServer for a given port.
//
// Concrete implementations: WiFiTransport, EthernetTransport.
// ---------------------------------------------------------------------------
class INetworkTransport {
public:
  virtual ~INetworkTransport() = default;

  // Initialise the hardware and start connecting.  Non-blocking: the caller
  // should check connected() in its loop and call maintainConnection() to
  // drive reconnect logic.
  virtual void begin() = 0;

  // Called every loop() iteration.  Implementations should use this to drive
  // DHCP renewals, reconnect attempts, etc.
  virtual void maintainConnection() = 0;

  // True when the link is up and an IP address has been assigned.
  virtual bool connected() const = 0;

  // Human-readable local IP address, e.g. "192.168.1.42".
  virtual String localIP() const = 0;

  // Active network name.  For WiFi this is the SSID; for Ethernet a fixed
  // label such as "Ethernet" is returned.
  virtual String networkName() const = 0;

  // Signal strength in dBm.  Ethernet implementations return 0.
  virtual int rssi() const = 0;

  // Create and return a server bound to `port`.  The caller owns the object.
  virtual std::unique_ptr<NetServer> createServer(uint16_t port) = 0;
};

#endif // NETWORK_TRANSPORT_H
