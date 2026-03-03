#ifndef TCP_SERVER_H
#define TCP_SERVER_H

#include <Arduino.h>
#include "net/network_client.h"
#include "net/network_server.h"
#include "config.h"
#include <memory>

class TcpServer {
public:
  explicit TcpServer(uint16_t port = Config::SERVER_PORT);

  // Initialise the server using an already-constructed NetServer.
  // Ownership of the server is transferred here.
  void begin(std::unique_ptr<NetServer> server);

  // Try to accept an incoming client.  Returns true if a client is connected
  // (either an existing live connection or a freshly accepted one).
  bool acceptClient();

  // Reference to the current client (only valid while acceptClient() == true).
  NetClient& client();

  // True when the client has a full packet (4-byte length header + payload)
  // waiting in the receive buffer.
  bool hasPacket();

  // Block-read one length-prefixed packet and return the payload string.
  // Only call when hasPacket() is true.
  String readPacket();

  // Send a length-prefixed packet to `client`.
  void sendPacket(NetClient& client, const String& payload);

private:
  uint16_t                   _port;
  std::unique_ptr<NetServer> _server;
  std::unique_ptr<NetClient> _client;
};

#endif // TCP_SERVER_H
