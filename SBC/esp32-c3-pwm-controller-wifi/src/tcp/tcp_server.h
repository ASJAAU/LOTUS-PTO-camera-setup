#ifndef TCP_SERVER_H
#define TCP_SERVER_H

#include <Arduino.h>
#include "i_net_client.h"
#include "i_net_server.h"
#include "config.h"
#include <memory>

// ---------------------------------------------------------------------------
// TcpServer — transport-agnostic TCP server.
//
// Unchanged public API vs. the original WiFi-only version:
//
//   TcpServer server;          // default port from Config::SERVER_PORT
//   server.begin();
//   if (server.acceptClient()) { ... }
//   if (server.hasPacket())    { String pkt = server.readPacket(); }
//   server.sendPacket(server.client(), payload);
//
// The only difference visible to callers is that client() now returns an
// INetClient& instead of a WiFiClient&.
// ---------------------------------------------------------------------------
class TcpServer {
public:
  explicit TcpServer(uint16_t port = Config::SERVER_PORT);

  // Initialise the server using an already-constructed INetServer.
  // Ownership of the server is transferred here.
  void begin(std::unique_ptr<INetServer> server);

  // Try to accept an incoming client.  Returns true if a client is connected
  // (either an existing live connection or a freshly accepted one).
  bool acceptClient();

  // Reference to the current client (only valid while acceptClient() == true).
  INetClient& client();

  // True when the client has a full packet (4-byte length header + payload)
  // waiting in the receive buffer.
  bool hasPacket();

  // Block-read one length-prefixed packet and return the payload string.
  // Only call when hasPacket() is true.
  String readPacket();

  // Send a length-prefixed packet to `client`.
  void sendPacket(INetClient& client, const String& payload);

private:
  uint16_t                   _port;
  std::unique_ptr<INetServer> _server;
  std::unique_ptr<INetClient> _client;
};

#endif // TCP_SERVER_H
