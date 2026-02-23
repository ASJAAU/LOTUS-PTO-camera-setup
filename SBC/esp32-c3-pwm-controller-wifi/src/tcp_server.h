#ifndef TCP_SERVER_H
#define TCP_SERVER_H

#include <Arduino.h>
#include <WiFi.h>
#include "config.h"

/**
 * Simple TCP server wrapper that implements the 4-byte big-endian
 * length-prefix framing used by the protocol.
 */
class TcpServer {
public:
  explicit TcpServer(uint16_t port = Config::SERVER_PORT);
  void begin();
  bool acceptClient();
  WiFiClient& client();
  bool hasPacket();
  String readPacket();
  void sendPacket(WiFiClient& client, const String& payload);

private:
  WiFiServer _server;
  WiFiClient _client;
};

#endif // TCP_SERVER_H
