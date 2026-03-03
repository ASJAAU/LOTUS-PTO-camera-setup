#ifndef WIFI_TRANSPORT_H
#define WIFI_TRANSPORT_H

#include <Arduino.h>
#include <WiFi.h>
#include "config.h"
#include "net/network_transport.h"
#include "net/network_client.h"
#include "net/network_server.h"
#include <memory>

class WiFiNetClient : public NetClient {
public:
  explicit WiFiNetClient(WiFiClient client) : _client(client) {}

  bool connected() const override {
    return const_cast<WiFiClient&>(_client) && const_cast<WiFiClient&>(_client).connected();
  }

  int available() override {
    return _client.available();
  }

  int read() override {
    return _client.read();
  }

  int read(uint8_t* buf, size_t len) override {
    return _client.read(buf, static_cast<int>(len));
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

private:
  WiFiClient _client;
};

class WiFiNetServer : public NetServer {
public:
  explicit WiFiNetServer(uint16_t port) : _server(port) {}

  void begin() override {
    _server.begin();
  }

  std::unique_ptr<NetClient> available() override {
    WiFiClient c = _server.available();
    if (!c) return nullptr;
    return std::unique_ptr<NetClient>(new WiFiNetClient(c));
  }

private:
  WiFiServer _server;
};

class WiFiTransport : public INetworkTransport {
public:
  void begin() override {
    WiFi.useStaticBuffers(true);
    WiFi.mode(WIFI_STA);
    WiFi.begin(Config::WIFI_SSID, Config::WIFI_PASS);

    Serial.print("Connecting to WiFi");
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED &&
           (millis() - start) < Config::TIMEOUT_MS) {
      delay(500);
      Serial.print('.');
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("WiFi connected");
    } else {
      Serial.println("WiFi initial connect failed; will retry in loop");
    }
  }

  void maintainConnection() override {
    if (WiFi.status() == WL_CONNECTED) return;

    unsigned long now = millis();
    if (now - _lastAttempt < Config::TIMEOUT_MS) return;
    _lastAttempt = now;

    Serial.println("WiFi disconnected — attempting reconnect");
    WiFi.begin(Config::WIFI_SSID, Config::WIFI_PASS);
  }

  bool connected() const override {
    return WiFi.status() == WL_CONNECTED;
  }

  String localIP() const override {
    return WiFi.localIP().toString();
  }

  String networkName() const override {
    return WiFi.SSID();
  }

  int rssi() const override {
    return WiFi.RSSI();
  }

  std::unique_ptr<NetServer> createServer(uint16_t port) override {
    return std::unique_ptr<NetServer>(new WiFiNetServer(port));
  }

private:
  unsigned long _lastAttempt = 0;
};

#endif // WIFI_TRANSPORT_H
