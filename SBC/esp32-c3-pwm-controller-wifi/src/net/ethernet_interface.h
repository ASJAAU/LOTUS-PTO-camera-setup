#ifndef ETHERNET_TRANSPORT_H
#define ETHERNET_TRANSPORT_H

#include "config.h"
#include <memory>
#include <Arduino.h>
#include <Ethernet.h>
#include "net/network_transport.h"
#include "net/network_client.h"
#include "net/network_server.h"

// ---------------------------------------------------------------------------
//   To use ethernet the following Config additions required in config.h:
//   static constexpr int    ETH_CS_PIN  = 5;   // SPI chip-select for the module
//   static constexpr bool   ETH_USE_DHCP = true;
//   // Static-IP fallback (used when ETH_USE_DHCP == false):
//   static constexpr uint8_t ETH_MAC[]  = {0xDE,0xAD,0xBE,0xEF,0xFE,0xED}; // MAC Adress
//   static constexpr uint8_t ETH_IP[]   = {192,168,1,100};                 // STATIC IP for fallback (DHCP fails or is unavailable)
//   static constexpr uint8_t ETH_DNS[]  = {8,8,8,8};                       // DNS Server
//   static constexpr uint8_t ETH_GW[]   = {192,168,1,1};                   // Gateway IP address (the server running dnsmasq)
//   static constexpr uint8_t ETH_MASK[] = {255,255,255,0};                 // Subnetmask
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// EthernetNetClient — NetClient backed by an EthernetClient
// ---------------------------------------------------------------------------
class EthernetNetClient : public NetClient {
public:
  explicit EthernetNetClient(EthernetClient client) : _client(client) {}

  bool connected() const override {
    // EthernetClient::connected() is not const in the Arduino library,
    // so we cast away const here for compatibility.
    return const_cast<EthernetClient&>(_client).connected();
  }

  int available() override {
    return _client.available();
  }

  int read() override {
    return _client.read();
  }

  int read(uint8_t* buf, size_t len) override {
    return _client.read(buf, len);
  }

  size_t write(const uint8_t* buf, size_t len) override {
    return _client.write(buf, len);
  }

  size_t print(const String& s) override {
    return _client.print(s);
  }

  void flush() override {
    _client.flush();
  }

  void stop() override {
    _client.stop();
  }

  explicit operator bool() const override {
    return static_cast<bool>(const_cast<EthernetClient&>(_client));
  }

private:
  EthernetClient _client;
};

// ---------------------------------------------------------------------------
// EthernetNetServer — NetServer backed by an EthernetServer
// ---------------------------------------------------------------------------
class EthernetNetServer : public NetServer {
public:
  explicit EthernetNetServer(uint16_t port) : _server(port) {}

  void begin() override {
    _server.begin();
  }

  std::unique_ptr<NetClient> available() override {
    EthernetClient c = _server.available();
    if (!c) return nullptr;
    return std::unique_ptr<INetClient>(new EthernetNetClient(c));
  }

private:
  EthernetServer _server;
};

class EthernetTransport : public INetworkTransport {
public:
  void begin() override {
    Ethernet.init(Config::ETH_CS_PIN);

    if (Config::ETH_USE_DHCP) {
      Serial.print("Ethernet: requesting DHCP address");
      if (Ethernet.begin(const_cast<uint8_t*>(Config::ETH_MAC)) == 0) {
        Serial.println("\nEthernet DHCP failed; will retry in loop");
        _connected = false;
        return;
      }
    } else {
      IPAddress ip(Config::ETH_IP[0],   Config::ETH_IP[1],
                   Config::ETH_IP[2],   Config::ETH_IP[3]);
      IPAddress dns(Config::ETH_DNS[0], Config::ETH_DNS[1],
                    Config::ETH_DNS[2], Config::ETH_DNS[3]);
      IPAddress gw(Config::ETH_GW[0],   Config::ETH_GW[1],
                   Config::ETH_GW[2],   Config::ETH_GW[3]);
      IPAddress mask(Config::ETH_MASK[0], Config::ETH_MASK[1],
                     Config::ETH_MASK[2], Config::ETH_MASK[3]);
      Ethernet.begin(const_cast<uint8_t*>(Config::ETH_MAC), ip, dns, gw, mask);
    }

    // Allow the hardware a moment to settle.
    delay(200);

    if (Ethernet.linkStatus() == LinkOFF) {
      Serial.println("Ethernet: no cable detected");
      _connected = false;
      return;
    }

    _connected = true;
    Serial.print("Ethernet connected, IP: ");
    Serial.println(Ethernet.localIP());
  }

  void maintainConnection() override {
    // Ethernet.maintain() handles DHCP lease renewal; call it every loop.
    Ethernet.maintain();

    bool linkUp = (Ethernet.linkStatus() != LinkOFF);
    if (!linkUp && _connected) {
      Serial.println("Ethernet: link lost");
      _connected = false;
    } else if (linkUp && !_connected) {
      Serial.println("Ethernet: link restored — re-initialising");
      begin();
    }
  }

  bool connected() const override {
    return _connected && (Ethernet.linkStatus() != LinkOFF);
  }

  String localIP() const override {
    return Ethernet.localIP().toString();
  }

  String networkName() const override {
    return "Ethernet";
  }

  int rssi() const override {
    return 0;   // Not applicable for wired connections.
  }

  std::unique_ptr<NetServer> createServer(uint16_t port) override {
    return std::unique_ptr<INetServer>(new EthernetNetServer(port));
  }

private:
  bool _connected = false;
};

#endif // ETHERNET_TRANSPORT_H
