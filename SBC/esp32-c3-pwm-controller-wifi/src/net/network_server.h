#ifndef I_NET_SERVER_H
#define I_NET_SERVER_H

#include <Arduino.h>
#include "net/network_client.h"
#include <memory>

// ---------------------------------------------------------------------------
// NetServer — transport-agnostic TCP server.
//
// Concrete implementations: WiFNetServer, EthernetNetServer.
// ---------------------------------------------------------------------------
class NetServer {
public:
  virtual ~NetServer() = default;

  // Start listening on the configured port.
  virtual void begin() = 0;

  // Returns a heap-allocated NetClient if an incoming connection is waiting,
  // or nullptr if there is nothing to accept.
  // Caller takes ownership (use std::unique_ptr or delete explicitly).
  virtual std::unique_ptr<NetClient> available() = 0;
};

#endif // I_NET_SERVER_H
