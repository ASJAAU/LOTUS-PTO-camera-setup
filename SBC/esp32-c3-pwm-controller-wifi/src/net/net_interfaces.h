#define NETWORK_CLIENT_H

#include <Arduino.h>

// ---------------------------------------------------------------------------
// NetClient — transport-agnostic wrapper around a connected TCP client.
// Concrete implementations: WiFNetClient, EthernetNetClient.
// TcpServer works exclusively through this interface so it never touches a
// WiFiClient or EthernetClient directly.
// ---------------------------------------------------------------------------
class NetClient {
public:
  virtual ~NetClient() = default;
  virtual bool connected() const = 0;
  virtual int available() = 0;
  virtual int read() = 0;
  virtual int read(uint8_t* buf, size_t len) = 0;
  virtual size_t write(const uint8_t* buf, size_t len) = 0;
  virtual size_t print(const String& s) = 0;
  virtual void flush() = 0;
  virtual void stop() = 0;
  // True if the client object refers to an accepted/connected socket at all
  // (may still be connected() == false if the peer closed the link).
  virtual explicit operator bool() const = 0;
};

class NetServer {
public:
  virtual ~NetServer() = default;
  virtual void begin() = 0;
  virtual std::unique_ptr<NetClient> available() = 0;
};
