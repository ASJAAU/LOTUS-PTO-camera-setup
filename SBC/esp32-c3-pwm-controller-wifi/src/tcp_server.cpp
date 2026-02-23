#include "tcp_server.h"
#include <Arduino.h>

TcpServer::TcpServer(uint16_t port)
  : _server(port) {
}

void TcpServer::begin() {
  _server.begin();
  Serial.printf("TCP server started on port %u\n", (unsigned)Config::SERVER_PORT);
}

bool TcpServer::acceptClient() {
  // If an existing client is present but disconnected, close it so
  // the server can accept a new incoming connection.
  if (_client && !_client.connected()) {
    Serial.println("Client disconnected, closing connection");
    _client.stop();
  }

  if (!_client || !_client.connected()) {
    WiFiClient newClient = _server.available();
    if (!newClient) return false;
    _client = newClient;
    Serial.println("Client connected");
  }
  return true;
}

WiFiClient& TcpServer::client() {
  return _client;
}

bool TcpServer::hasPacket() {
  if (!_client || !_client.connected()) return false;
  return _client.available() >= 4;
}

String TcpServer::readPacket() {
  uint8_t header[4];
  _client.read(header, 4);
  uint32_t length = (header[0] << 24) | (header[1] << 16) | (header[2] << 8) | header[3];

  String payload;
  payload.reserve(length + 1);
  while (payload.length() < length) {
    if (_client.available()) {
      payload += (char)_client.read();
    }
  }
  return payload;
}

void TcpServer::sendPacket(WiFiClient& client, const String& payload) {
  uint32_t length = payload.length();
  uint8_t header[4] = {
    (uint8_t)((length >> 24) & 0xFF),
    (uint8_t)((length >> 16) & 0xFF),
    (uint8_t)((length >> 8) & 0xFF),
    (uint8_t)(length & 0xFF)
  };
  client.write(header, 4);
  client.print(payload);
}
