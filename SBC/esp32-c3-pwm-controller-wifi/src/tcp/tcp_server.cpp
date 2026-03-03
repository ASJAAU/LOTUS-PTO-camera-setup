#include "tcp_server.h"
#include <Arduino.h>

TcpServer::TcpServer(uint16_t port)
  : _port(port) {
  // _server is supplied later via begin(std::unique_ptr<INetServer>).
  // This keeps TcpServer decoupled from NetworkManager construction order.
}

void TcpServer::begin(std::unique_ptr<INetServer> server) {
  _server = std::move(server);
  _server->begin();
  Serial.printf("TCP server started on port %u\n", (unsigned)_port);
}

bool TcpServer::acceptClient() {
  // If the existing client has gone away, release it so we can accept anew.
  if (_client && !_client->connected()) {
    Serial.println("Client disconnected, closing connection");
    _client->stop();
    _client.reset();
  }

  if (!_client) {
    std::unique_ptr<INetClient> incoming = _server->available();
    if (!incoming) return false;
    _client = std::move(incoming);
    Serial.println("Client connected");
  }

  return true;
}

INetClient& TcpServer::client() {
  return *_client;
}

bool TcpServer::hasPacket() {
  if (!_client || !_client->connected()) return false;
  return _client->available() >= 4;
}

String TcpServer::readPacket() {
  uint8_t header[4];
  _client->read(header, 4);
  uint32_t length = ((uint32_t)header[0] << 24)
                  | ((uint32_t)header[1] << 16)
                  | ((uint32_t)header[2] <<  8)
                  |  (uint32_t)header[3];

  String payload;
  payload.reserve(length + 1);
  while ((uint32_t)payload.length() < length) {
    if (_client->available()) {
      payload += (char)_client->read();
    }
  }
  return payload;
}

void TcpServer::sendPacket(INetClient& client, const String& payload) {
  uint32_t length = payload.length();
  uint8_t header[4] = {
    (uint8_t)((length >> 24) & 0xFF),
    (uint8_t)((length >> 16) & 0xFF),
    (uint8_t)((length >>  8) & 0xFF),
    (uint8_t)( length        & 0xFF)
  };
  client.write(header, 4);
  client.print(payload);
}
