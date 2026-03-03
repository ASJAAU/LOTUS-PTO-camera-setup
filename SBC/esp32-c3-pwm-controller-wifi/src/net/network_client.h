#ifndef NETWORK_CLIENT_H
#define NETWORK_CLIENT_H

#include <Arduino.h>

// ---------------------------------------------------------------------------
// NetClient — transport-agnostic wrapper around a connected TCP client.
//
// Concrete implementations: WiFNetClient, EthernetNetClient.
// TcpServer works exclusively through this interface so it never touches a
// WiFiClient or EthernetClient directly.
// ---------------------------------------------------------------------------
class NetClient {
public:
  virtual ~NetClient() = default;

  // Returns true if the underlying client object is valid AND still connected.
  virtual bool connected() const = 0;

  // Number of bytes waiting in the receive buffer.
  virtual int available() = 0;

  // Read a single byte (-1 on failure).
  virtual int read() = 0;

  // Read up to `len` bytes into `buf`. Returns bytes actually read.
  virtual int read(uint8_t* buf, size_t len) = 0;

  // Write `len` bytes from `buf`. Returns bytes written.
  virtual size_t write(const uint8_t* buf, size_t len) = 0;

  // Write a String (convenience wrapper).
  virtual size_t print(const String& s) = 0;

  // Flush the send buffer.
  virtual void flush() = 0;

  // Close the connection and release resources.
  virtual void stop() = 0;
};

#endif // I_NET_CLIENT_H
